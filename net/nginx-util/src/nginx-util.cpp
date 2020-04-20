#include <iostream>

#include "nginx-util.hpp"

#ifndef NO_SSL
#include "nginx-ssl-util.hpp"
#endif


static auto constexpr file_comment_auto_created = std::string_view
    {"# This file is re-created when Nginx starts or a local IP is changed.\n"};


// TODO(pst) replace it with blobmsg_get_string if upstream takes const:
#ifndef NO_UBUS
static inline auto _pst_get_string(const blob_attr *attr) -> char *
{ return static_cast<char *>(blobmsg_data(attr)); }
#endif


auto create_lan_listen() -> std::vector<std::string>
{
    std::vector<std::string> ips;

#ifndef NO_UBUS
    try {
        auto loopback_status=ubus::call("network.interface.loopback", "status");

        for (auto ip : loopback_status.filter("ipv4-address", "", "address"))
        { ips.emplace_back(_pst_get_string(ip)); }

        for (auto ip : loopback_status.filter("ipv6-address", "", "address"))
        { ips.emplace_back(std::string{"["} + _pst_get_string(ip) + "]"); }

    } catch (const std::runtime_error &) { /* do nothing about it */ }

    try {
        auto lan_status = ubus::call("network.interface.lan", "status");

        for (auto ip : lan_status.filter("ipv4-address", "", "address"))
        { ips.emplace_back(_pst_get_string(ip)); }

        for (auto ip : lan_status.filter("ipv6-address", "", "address"))
        { ips.emplace_back(std::string{"["} + _pst_get_string(ip) + "]"); }

        for (auto ip : lan_status.filter("ipv6-prefix-assignment", "",
            "local-address", "address"))
        { ips.emplace_back(std::string{"["} + _pst_get_string(ip) + "]"); }

    } catch (const std::runtime_error &) { /* do nothing about it */ }
#else
    ips.emplace_back("127.0.0.1");
#endif

    std::string listen = std::string{file_comment_auto_created};
    std::string listen_default = std::string{file_comment_auto_created};
    for (const auto & ip : ips) {
        listen += "\tlisten " + ip + ":80;\n";
        listen_default += "\tlisten " + ip + ":80 default_server;\n";
    }
    write_file(LAN_LISTEN, listen);
    write_file(LAN_LISTEN_DEFAULT, listen_default);

#ifndef NO_SSL
    std::string ssl_listen = std::string{file_comment_auto_created};
    std::string ssl_listen_default = std::string{file_comment_auto_created};
    for (const auto & ip : ips) {
        ssl_listen += "\tlisten " + ip + ":443 ssl;\n";
        ssl_listen_default += "\tlisten " + ip + ":443 ssl default_server;\n";
    }
    write_file(LAN_SSL_LISTEN, ssl_listen);
    write_file(LAN_SSL_LISTEN_DEFAULT, ssl_listen_default);
#endif

    return ips;
}


inline auto change_if_starts_with(const std::string_view & subject,
                                   const std::string_view & prefix,
                                   const std::string_view & substitute,
                                   const std::string_view & seperator=" \t\n;")
    -> std::string
{
    auto view = subject;
    view = view.substr(view.find_first_not_of(seperator));
    if (view.rfind(prefix, 0)==0) {
        if (view.size()==prefix.size()) { return std::string{substitute}; }
        view = view.substr(prefix.size());
        if (seperator.find(view[0])!=std::string::npos) {
            auto ret = std::string{substitute};
            ret += view;
            return ret;
        }
    }
    return std::string{subject};
}


inline auto create_server_conf(const uci::section & sec,
                               const std::vector<std::string> & ips,
                               const std::string & indent="") -> std::string
{
        auto secname = sec.name();

        auto legacypath = std::string{CONF_DIR} + secname + ".conf";
        if (access(legacypath.c_str(), R_OK)==0) {

            auto message = std::string{"skipped UCI server 'nginx."} + secname;
            message += "' as it could conflict with: " + legacypath + "\n";

            //TODO(pst) std::cerr<<"create_server_conf notice: "<<message;

            return indent + "# " + message;
        } //else:

        auto conf = indent + "server { #see uci show 'nginx." + secname + "'\n";

        for (auto opt : sec) {

            for (auto itm : opt) {

                if (opt.name()==LISTEN_LOCALLY) {
                    for (const auto & ip : ips) {
                        conf += indent + "\tlisten ";
                        conf += ip + ":" + itm.name() + ";\n";
                    }
                }

                if (opt.name().rfind("uci_", 0)==0) { continue; }
                //else: standard opt.name()

                auto val = itm.name();

                if (opt.name()=="error_log")
                { val = change_if_starts_with(val, "logd", "/proc/self/fd/1"); }

                else if (opt.name()=="access_log")
                { val = change_if_starts_with(val, "logd", "stderr"); }

                conf += indent + "\t" + opt.name() + " " + itm.name() + ";\n";
            }
        }

        conf += indent + "}\n";

        return conf;
}


void init_uci(const uci::package & pkg, const std::vector<std::string> & ips)
{
    auto conf = std::string{file_comment_auto_created};
    auto suffix = std::string{};
    auto indent = std::string{};

    {
        const auto tmpl = read_file(std::string{UCI_CONF}+".template");

        const auto uci_http_config = std::string_view{"#UCI_HTTP_CONFIG\n"};

        /* regex needs PCRE or std::regex (only used in SSL version):
         * #ifndef NO_SSL
         * rgx::smatch match;
         * auto rgx_http_config = rgx::regex{R"(^(\s*)"+uci_http_config+")"};
         * if (rgx::regex_search(tmpl, match, rgx_http_config)) {
         *      auto pos = tmpl.begin() + match.position(0);
         *      conf.append(tmpl.begin(), pos);
         *      indent = match.str(1);
         *      suffix.assign(pos+match.length(0), tmpl.end());
         * }
         * #else
         */
        auto pos = tmpl.find(uci_http_config);
        if (pos != std::string::npos) {
            const auto index = tmpl.find_last_not_of(" \t", pos-1);
            const auto before = tmpl.begin() + index + 1;
            conf.append(tmpl.begin(), before);
            const auto middle = tmpl.begin() + pos;
            indent.assign(before, middle);
            const auto after = middle + uci_http_config.length();
            suffix.assign(after, tmpl.end());
        }
        /*#endif*/
        else {
            write_file(VAR_UCI_CONF, tmpl);
            return;
        }
    }

    for (auto sec : pkg) {
        if (sec.type()==std::string_view{"server"}) {
            conf += create_server_conf(sec, ips, indent) + "\n";
        }
    }

    conf += suffix;
    write_file(VAR_UCI_CONF, conf);
}


auto is_enabled(const uci::package & pkg) -> bool {
    for (auto sec : pkg) {
        if (sec.type()!=std::string_view{"main"}) { continue; }
        if (sec.name()!=std::string_view{"global"}) { continue; }
        for (auto opt : sec) {
            if (opt.name()!="uci_enable") { continue; }
            for (auto itm : opt) {
                if (itm) { return true; }
            }
        }
    }
    return false;
}


/*
 * ___________main_thread________________|______________thread_1________________
 *  ips = create_lan_listen()            | config = uci::package("nginx")
 *  if config_enabled (set in thread_1): | config_enabled = is_enabled(config)
 *  then init_uci(config, ips)           | check_ssl(config, config_enabled)
 */
void init_lan()
{
    std::exception_ptr ex;
    std::unique_ptr<uci::package> config;
    bool config_enabled = false;
    std::mutex configuring;

    configuring.lock();
    auto thrd = std::thread([&config, &config_enabled, &configuring, &ex]{
        try {
            config = std::make_unique<uci::package>("nginx");
            config_enabled = is_enabled(*config);
            configuring.unlock();
#ifndef NO_SSL
            check_ssl(*config, config_enabled);
#endif
        } catch (...) {
            std::cerr<<"init_lan error: checking UCI file /etc/config/nginx\n";
            ex = std::current_exception();
        }
    });

    std::vector<std::string> ips;
    try { ips = create_lan_listen(); }
    catch (...) {
        std::cerr<<"init_lan error: cannot create listen files of local IPs.\n";
        ex = std::current_exception();
    }

    configuring.lock();
    if (config_enabled) {
        try { init_uci(*config, ips); }
        catch (...) {
            std::cerr<<"init_lan error: cannot create "<<VAR_UCI_CONF<<" from ";
            std::cerr<<UCI_CONF<<".template using UCI file /etc/config/nginx\n";
            ex = std::current_exception();
        }
    }

    thrd.join();
    if (ex) { std::rethrow_exception(ex); }
}


void get_env()
{
    std::cout<<"UCI_CONF="<<"'"<<UCI_CONF<<"'"<<std::endl;
    std::cout<<"NGINX_CONF="<<"'"<<NGINX_CONF<<"'"<<std::endl;
    std::cout<<"CONF_DIR="<<"'"<<CONF_DIR<<"'"<<std::endl;
    std::cout<<"LAN_NAME="<<"'"<<LAN_NAME<<"'"<<std::endl;
    std::cout<<"LAN_LISTEN="<<"'"<<LAN_LISTEN<<"'"<<std::endl;
#ifndef NO_SSL
    std::cout<<"LAN_SSL_LISTEN="<<"'"<<LAN_SSL_LISTEN<<"'"<<std::endl;
    std::cout<<"SSL_SESSION_CACHE_ARG="<<"'"<<SSL_SESSION_CACHE_ARG(LAN_NAME)<<
        "'"<<std::endl;
    std::cout<<"SSL_SESSION_TIMEOUT_ARG="<<"'"<<SSL_SESSION_TIMEOUT_ARG<<"'\n";
    std::cout<<"ADD_SSL_FCT="<<"'"<<ADD_SSL_FCT<<"'"<<std::endl;
    std::cout<<"MANAGE_SSL="<<"'"<<MANAGE_SSL<<"'"<<std::endl;
#endif
    std::cout<<"LISTEN_LOCALLY="<<"'"<<LISTEN_LOCALLY<<"'"<<std::endl;
}


auto main(int argc, char * argv[]) -> int
{
    // TODO(pst): use std::span when available:
    auto args = std::basic_string_view{argv, static_cast<size_t>(argc)};

    auto cmds = std::array{
        std::array<std::string_view, 2>{"init_lan", ""},
        std::array<std::string_view, 2>{"get_env", ""},
#ifndef NO_SSL
        std::array<std::string_view, 2>{ADD_SSL_FCT, "server_name" },
        std::array<std::string_view, 2>{"del_ssl", "server_name" },
        std::array<std::string_view, 2>{"check_ssl", "" },
#endif
    };

    try {

        if (argc==2 && args[1]==cmds[0][0]) {init_lan();}

        else if (argc==2 && args[1]==cmds[1][0]) { get_env(); }

#ifndef NO_SSL
        else if (argc==3 && args[1]==cmds[2][0])
        { add_ssl_if_needed(std::string{args[2]});}

        else if (argc==3 && args[1]==cmds[3][0])
        { del_ssl(std::string{args[2]}); }

        else if (argc==2 && args[1]==cmds[3][0]) //TODO(pst) deprecate
        {
            try {
                auto name = std::string{LAN_NAME};
                if (del_ssl_legacy(name)) {
                    auto crtpath = std::string{CONF_DIR} + name + ".crt";
                    remove(crtpath.c_str());
                    auto keypath = std::string{CONF_DIR} + name + ".key";
                    remove(keypath.c_str());
                }
            } catch (...) { /* do nothing. */ }
        }

        else if (argc==2 && args[1]==cmds[4][0])
        { check_ssl(uci::package{"nginx"}); }
#endif

        else {
            std::cerr<<"Tool for creating Nginx configuration files (";
#ifdef VERSION
            std::cerr<<"version "<<VERSION<<" ";
#endif
            std::cerr<<"with libuci, ";
#ifndef NO_UBUS
            std::cerr<<"libubus, ";
#endif
#ifndef NO_SSL
            std::cerr<<"libopenssl, ";
#ifndef NO_PCRE
            std::cerr<<"PCRE, ";
#endif
#endif
            std::cerr<<"pthread and libstdcpp)."<<std::endl;

            auto usage = std::string{"usage: "} + *argv + " [";
            for (auto cmd : cmds) {
                usage += cmd[0];
                if (!cmd[1].empty()) { usage += " "; }
                usage += std::string{cmd[1]} += "|";
            }
            usage[usage.size()-1] = ']';
            std::cerr<<usage<<std::endl;

            throw std::runtime_error("main error: argument not recognized");
        }

        return 0;

    }

    catch (const std::exception & e) {
        std::cerr<<" * "<<*argv<<" "<<e.what()<<"\n";
    }

    catch (...) {
        std::cerr<<" * * "<<*argv;
        perror(" main error");
    }

    return 1;

}
