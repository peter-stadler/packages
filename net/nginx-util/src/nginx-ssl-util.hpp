#ifndef __NGINX_SSL_UTIL_HPP
#define __NGINX_SSL_UTIL_HPP

#ifdef NO_PCRE
#include <regex>
namespace rgx = std;
#else
#include "regex-pcre.hpp"
#endif

#include "nginx-util.hpp"
#include "px5g-openssl.hpp"


#ifndef NO_UBUS
static constexpr auto UBUS_TIMEOUT = 1000;
#endif

// once a year:
static constexpr auto CRON_INTERVAL = std::string_view{"3 3 12 12 *"};

static constexpr auto LAN_SSL_LISTEN =
    std::string_view{"/var/lib/nginx/lan_ssl.listen"};

static constexpr auto LAN_SSL_LISTEN_DEFAULT = //TODO(pst) deprecate
    std::string_view{"/var/lib/nginx/lan_ssl.listen.default"};

static constexpr auto ADD_SSL_FCT = std::string_view{"add_ssl"};

static constexpr auto SSL_SESSION_CACHE_ARG =
    [](const std::string_view & /*name*/) -> std::string
    { return "shared:SSL:32k"; };

static constexpr auto SSL_SESSION_TIMEOUT_ARG = std::string_view{"64m"};


using _Line =
    std::array< std::string (*)(const std::string &, const std::string &), 2 >;

class Line {

private:

    _Line _line;

public:

    explicit Line(const _Line & line) noexcept : _line{line} {}

    template<const _Line & ...xn>
    static auto build() noexcept -> Line
    {
        return Line{_Line{
            [](const std::string & p, const std::string & b) -> std::string
            { return (... + xn[0](p, b)); },
            [](const std::string & p, const std::string & b) -> std::string
            { return (... + xn[1](p, b)); }
        }};
    }


    [[nodiscard]] auto STR(const std::string & param, const std::string & begin)
        const -> std::string
    { return _line[0](param, begin); }


    [[nodiscard]] auto RGX() const -> rgx::regex
    { return rgx::regex{_line[1]("", "")}; }

};


auto get_if_missed(const std::string & conf, const Line & LINE,
                   const std::string & val,
                   const std::string & indent="\n    ", bool compare=true)
    -> std::string;


auto delete_if(const std::string & conf, const rgx::regex & rgx,
               const std::string & val="")
    -> std::string;


void check_ssl_certificate(const std::string & crtpath,
                           const std::string & keypath);


void install_cron_job(const Line & CRON_LINE, const std::string & name="");


void add_ssl_if_needed(const std::string & name);


void remove_cron_job(const Line & CRON_LINE, const std::string & name="");


auto del_ssl_legacy(const std::string & name) -> bool;


void del_ssl(const std::string & name);


void check_ssl(const uci::package & pkg);


constexpr auto _begin = _Line{
    [](const std::string & /*param*/, const std::string & begin) -> std::string
    { return begin; },

    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    { return R"([{;](?:\s*#[^\n]*(?=\n))*(\s*))"; }
};


constexpr auto _space = _Line{
    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    { return std::string{" "}; },

    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    { return R"(\s+)"; }
};


constexpr auto _newline = _Line{
    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    { return std::string{"\n"}; },

    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    { return std::string{"\n"}; }
};


constexpr auto _end = _Line{
    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    { return std::string{";"}; },

    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    { return std::string{R"(\s*;(?:[\t ]*#[^\n]*)?)"}; }
};


template<char clim='\0'>
constexpr auto _capture = _Line{
    [](const std::string & param, const std::string & /*begin*/) -> std::string
    { return '\'' + param + '\''; },

    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    {
        const auto lim = clim=='\0' ? std::string{"\\s"} : std::string{clim};
        return std::string{R"(((?:(?:"[^"]*")|(?:[^'")"} +
            lim + "][^" + lim + "]*)|(?:'[^']*'))+)";
    }
};


template<const std::string_view & strptr, char clim='\0'>
constexpr auto _escape = _Line{
    [](const std::string &  /*param*/, const std::string & /*begin*/)
        -> std::string
    {
        return clim=='\0' ?
            std::string{strptr.data()} :
            clim + std::string{strptr.data()} + clim;
    },

    [](const std::string & /*param*/, const std::string & /*begin*/)
        -> std::string
    {
        std::string ret{};
        for (char c : strptr) {
                switch(c) {
                    case '^': ret += '\\'; [[fallthrough]];
                    case '_': [[fallthrough]];
                    case '-': ret += c;
                    break;
                    default:
                        if ((isalpha(c)!=0) || (isdigit(c)!=0)) { ret += c; }
                        else { ret += std::string{"["}+c+"]"; }
                }
        }
        return "(?:"+ret+"|'"+ret+"'"+"|\""+ret+"\""+")";
    }
};


constexpr std::string_view _check_ssl = "check_ssl";

constexpr std::string_view _server_name = "server_name";

constexpr std::string_view _include = "include";

constexpr std::string_view _ssl_certificate = "ssl_certificate";

constexpr std::string_view _ssl_certificate_key = "ssl_certificate_key";

constexpr std::string_view _ssl_session_cache = "ssl_session_cache";

constexpr std::string_view _ssl_session_timeout = "ssl_session_timeout";


// For a compile time regex lib, this must be fixed, use one of these options:
// * Hand craft or macro concat them (loosing more or less flexibility).
// * Use Macro concatenation of __VA_ARGS__ with the help of:
//   https://p99.gforge.inria.fr/p99-html/group__preprocessor__for.html
// * Use constexpr---not available for strings or char * for now---look at lib.

static const auto CRON_CHECK = Line::build
    <_space, _escape<NGINX_UTIL>, _space, _escape<_check_ssl,'\''>, _newline>();

static const auto CRON_CMD = Line::build
    <_space, _escape<NGINX_UTIL>, _space, _escape<ADD_SSL_FCT,'\''>, _space,
        _capture<>, _newline>();

static const auto NGX_SERVER_NAME =
    Line::build<_begin, _escape<_server_name>, _space, _capture<';'>, _end>();

static const auto NGX_INCLUDE_LAN_LISTEN = Line::build
    <_begin, _escape<_include>, _space, _escape<LAN_LISTEN,'\''>, _end>();

static const auto NGX_INCLUDE_LAN_LISTEN_DEFAULT = Line::build
    <_begin, _escape<_include>, _space,
        _escape<LAN_LISTEN_DEFAULT, '\''>, _end>();

static const auto NGX_INCLUDE_LAN_SSL_LISTEN = Line::build
    <_begin, _escape<_include>, _space, _escape<LAN_SSL_LISTEN, '\''>, _end>();

static const auto NGX_INCLUDE_LAN_SSL_LISTEN_DEFAULT = Line::build
    <_begin, _escape<_include>, _space,
        _escape<LAN_SSL_LISTEN_DEFAULT, '\''>, _end>();

static const auto NGX_SSL_CRT = Line::build
    <_begin, _escape<_ssl_certificate>, _space, _capture<';'>, _end>();

static const auto NGX_SSL_KEY = Line::build
    <_begin, _escape<_ssl_certificate_key>, _space, _capture<';'>, _end>();

static const auto NGX_SSL_SESSION_CACHE = Line::build
    <_begin, _escape<_ssl_session_cache>, _space, _capture<';'>, _end>();

static const auto NGX_SSL_SESSION_TIMEOUT = Line::build
    <_begin, _escape<_ssl_session_timeout>, _space, _capture<';'>, _end>();



// ------------------------- implementation: ----------------------------------


auto get_if_missed(const std::string & conf, const Line & LINE,
                   const std::string & val,
                   const std::string & indent, bool compare)
    -> std::string
{
    if (!compare || val.empty()) {
        return rgx::regex_search(conf, LINE.RGX()) ? "" : LINE.STR(val, indent);
    }

    rgx::smatch match; // assuming last capture has the value!

    for (auto pos = conf.begin();
         rgx::regex_search(pos, conf.end(), match, LINE.RGX());
         pos += match.position(0) + match.length(0))
    {
        const std::string value = match.str(match.size() - 1);

        if (value==val || value=="'"+val+"'" || value=='"'+val+'"') {
            return "";
        }
    }

    return LINE.STR(val, indent);
}


auto delete_if(const std::string & conf, const rgx::regex & rgx,
               const std::string & val)
    -> std::string
{
    std::string ret{};
    auto pos = conf.begin();

    for (rgx::smatch match;
         rgx::regex_search(pos, conf.end(), match, rgx);
         pos += match.position(0) + match.length(0))
    {
        const std::string value = match.str(match.size() - 1);
        auto len = match.position( match.size()>1 ? 1 : 0 );
        bool compare = !val.empty();
        if (compare && value!=val && value!="'"+val+"'" && value!='"'+val+'"') {
            len = match.position(0) + match.length(0);
        }
        ret.append(pos, pos + len);
    }

    ret.append(pos, conf.end());
    return ret;
}


inline void add_ssl_directives_to(const std::string & name, const bool isdefault)
{
    const std::string prefix = std::string{CONF_DIR} + name;

    std::string conf = read_file(prefix+".conf");

    const std::string & const_conf = conf; // iteration needs const string.
    rgx::smatch match; // captures str(1)=indentation spaces, str(2)=server name
    for (auto pos = const_conf.begin();
        rgx::regex_search(pos, const_conf.end(), match, NGX_SERVER_NAME.RGX());
        pos += match.position(0) + match.length(0))
    {
        if (match.str(2).find(name) == std::string::npos) { continue; } //else:

        const std::string indent = match.str(1);

        std::string adds = isdefault ?
            get_if_missed(conf, NGX_INCLUDE_LAN_SSL_LISTEN_DEFAULT,"",indent) :
            get_if_missed(conf, NGX_INCLUDE_LAN_SSL_LISTEN, "", indent);

        adds += get_if_missed(conf, NGX_SSL_CRT, prefix+".crt", indent);

        adds += get_if_missed(conf, NGX_SSL_KEY, prefix+".key", indent);

        adds += get_if_missed(conf, NGX_SSL_SESSION_CACHE,
                              SSL_SESSION_CACHE_ARG(name), indent, false);

        adds += get_if_missed(conf, NGX_SSL_SESSION_TIMEOUT,
                        std::string{SSL_SESSION_TIMEOUT_ARG}, indent, false);

        if (adds.length() > 0) {
            pos += match.position(0) + match.length(0);

            conf = std::string(const_conf.begin(), pos) + adds +
                    std::string(pos, const_conf.end());

            conf = isdefault ?
                delete_if(conf, NGX_INCLUDE_LAN_LISTEN_DEFAULT.RGX()) :
                delete_if(conf, NGX_INCLUDE_LAN_LISTEN.RGX());

            write_file(prefix+".conf", conf);

            std::cerr<<"Added SSL directives to "<<prefix<<".conf: ";
            std::cerr<<adds<<std::endl;
        }

        return;
    }

    auto errmsg = std::string{"add_ssl_directives_to error: "};
    errmsg += "cannot add SSL directives to " + name + ".conf, missing: ";
    errmsg += NGX_SERVER_NAME.STR(name, "\n    ") + "\n";
    throw std::runtime_error(errmsg.c_str());
}


template<typename T>
inline auto num2hex(T bytes) -> std::array<char, 2*sizeof(bytes)+1>
{
    constexpr auto n = 2*sizeof(bytes);
    std::array<char, n+1> str{};

    for (size_t i=0; i<n; ++i) {
        static const std::array<char, 17> hex{"0123456789ABCDEF"};
        static constexpr auto get = 0x0fU;
        str.at(i) = hex.at(bytes & get);

        static constexpr auto move = 4U;
        bytes >>= move;
    }

    str[n] = '\0';
    return str;
}


template<typename T>
inline auto get_nonce(const T salt=0) -> T
{
    T nonce = 0;

    std::ifstream urandom{"/dev/urandom"};

    static constexpr auto move = 6U;

    constexpr size_t steps = (sizeof(nonce)*8 - 1)/move + 1;

    for (size_t i=0; i<steps; ++i) {
        if (!urandom.good()) { throw std::runtime_error("get_nonce error"); }
        nonce = (nonce << move) + static_cast<unsigned>(urandom.get());
    }

    nonce ^= salt;

    return nonce;
}


inline void create_ssl_certificate(const std::string & crtpath,
                                   const std::string & keypath,
                                   const int days=792)
{
    size_t nonce = 0;

    try { nonce = get_nonce(nonce); }

    catch (...) { // the address of a variable should be random enough:
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) sic:
        nonce += reinterpret_cast<size_t>(&crtpath);
    }

    auto noncestr = num2hex(nonce);

    const auto tmpcrtpath = crtpath + ".new-" + noncestr.data();
    const auto tmpkeypath = keypath + ".new-" + noncestr.data();

    try {
        auto pkey = gen_eckey(NID_secp384r1);

        write_key(pkey, tmpkeypath);

        std::string subject {"/C=ZZ/ST=Somewhere/L=None/CN=OpenWrt/O=OpenWrt"};
        subject += noncestr.data();

        selfsigned(pkey, days, subject, tmpcrtpath);

        static constexpr auto to_seconds = 24*60*60;
        static constexpr auto leeway = 42;
        if (!checkend(tmpcrtpath, days*to_seconds - leeway)) {
            throw std::runtime_error("bug: created certificate is not valid!!");
        }

    } catch (...) {
        std::cerr<<"create_ssl_certificate error: ";
        std::cerr<<"cannot create selfsigned certificate, ";
        std::cerr<<"removing temporary files ..."<<std::endl;

        if (remove(tmpcrtpath.c_str())!=0) {
            auto errmsg = "\t cannot remove "+tmpcrtpath;
            perror(errmsg.c_str());
        }

        if (remove(tmpkeypath.c_str())!=0) {
            auto errmsg = "\t cannot remove "+tmpkeypath;
            perror(errmsg.c_str());
        }

        throw;
    }

    if ( rename(tmpcrtpath.c_str(), crtpath.c_str())!=0 ||
         rename(tmpkeypath.c_str(), keypath.c_str())!=0 )
    {
        auto errmsg = std::string{"create_ssl_certificate warning: "};
        errmsg += "cannot move "+tmpcrtpath+" to "+crtpath;
        errmsg += " or "+tmpkeypath+" to "+keypath+", continuing ... ";
        perror(errmsg.c_str());
    }

    std::cerr<<"Created self-signed SSL certificate '"<<crtpath;
    std::cerr<<"' with key '"<<keypath<<"'.\n";
}


inline void check_ssl_certificate(const std::string & crtpath,
                                  const std::string & keypath)
{
    constexpr auto remaining_seconds = (365 + 32)*24*60*60;
    constexpr auto validity_days = 3*(365 + 31);

    bool is_valid = true;

    if (access(keypath.c_str(), R_OK) != 0 ||
        access(crtpath.c_str(), R_OK) != 0)
    { is_valid = false; }

    else {
        try {
            if (!checkend(crtpath, remaining_seconds)) {
                is_valid = false;
            }
        }
        catch (...) { // something went wrong, maybe it is in DER format:
            try {
                if (!checkend(crtpath, remaining_seconds, false)) {
                    is_valid = false;
                }
            }
            catch (...) { // it has neither DER nor PEM format, rebuild.
                is_valid = false;
            }
        }
    }

    if (!is_valid) { create_ssl_certificate(crtpath, keypath, validity_days); }
}


inline auto add_ssl_to_config(const std::string & name)
{
    auto sec = uci::package{"nginx"}[name]; // let it throw.

    struct { std::string crt; std::string key; } ret;

    auto cache = false;
    auto timeout = false;
    for (auto opt : sec) {

        if (opt.name()=="ssl_session_cache") { cache = true; continue; } //else:

        if (opt.name()=="ssl_session_timeout") { timeout=true; continue; }

        //else:
        for (auto itm : opt) {

            if (opt.name()=="ssl_certificate_key") { ret.key = itm.name(); }

            else if (opt.name()=="ssl_certificate") { ret.crt = itm.name(); }

            else if (opt.name()=="listen" || opt.name()==LISTEN_LOCALLY) {
                static const auto rgx_port =
                    rgx::regex{R"(^\s*([^:]*:|\[[^]]*\]:)?80(\s|$|;))"};
                auto val = regex_replace(itm.name(), rgx_port, "$01443 ssl$2");
                itm.rename(val.c_str());
            }
        }
    }

    sec.set(MANAGE_SSL.data(), "true");

    if (ret.crt.empty()) {
        ret.crt = std::string{CONF_DIR} + name + ".crt";
        sec.set("ssl_certificate", ret.crt.c_str());
    }

    if (ret.key.empty()) {
        ret.key = std::string{CONF_DIR} + name + ".key";
        sec.set("ssl_certificate_key", ret.key.c_str());
    }

    if (!cache)
    { sec.set("ssl_session_cache", SSL_SESSION_CACHE_ARG(name).data()); }

    if (!timeout)
    { sec.set("ssl_session_timeout", SSL_SESSION_TIMEOUT_ARG.data()); }

    sec.commit();

    return ret;
}


void install_cron_job(const Line & CRON_LINE, const std::string & name)
{
    static const char * filename = "/etc/crontabs/root";

    std::string conf{};
    try { conf = read_file(filename); }
    catch (const std::ifstream::failure &) { /* is ok if not found, create. */ }

    const std::string add = get_if_missed(conf, CRON_LINE, name);

    if (add.length() > 0) {
#ifndef NO_UBUS
        auto service = ubus::call("service","list",UBUS_TIMEOUT).filter("cron");

        if (!service) {
            std::string errmsg{"install_cron_job error: "};
            errmsg += "Cron unavailable to re-create the ssl certificate";
            errmsg += (name.empty() ? std::string{"s\n"} : " for '"+name+"'\n");
            throw std::runtime_error(errmsg.c_str());
        } //else active with or without instances:
#endif

        write_file(filename, std::string{CRON_INTERVAL}+add, std::ios::app);

#ifndef NO_UBUS
        call("/etc/init.d/cron", "reload");
#endif

        std::cerr<<"Rebuild the self-signed SSL certificate";
        std::cerr<<(name.empty() ? std::string{"s"} : " for '"+name+"'");
        std::cerr<<" annually with cron."<<std::endl;
    }
}


void add_ssl_if_needed(const std::string & name)
{
    const auto legacypath = std::string{CONF_DIR} + name + ".conf";
    if (access(legacypath.c_str(), R_OK)==0) {
        add_ssl_directives_to(name, name==LAN_NAME); // let it throw.

        const auto crtpath = std::string{CONF_DIR} + name + ".crt";
        const auto keypath = std::string{CONF_DIR} + name + ".key";
        check_ssl_certificate(crtpath, keypath); // let it throw.

        try { install_cron_job(CRON_CMD, name); }
        catch (...) {
            std::cerr<<"add_ssl_if_needed warning: cannot use cron to rebuild ";
            std::cerr<<"the self-signed SSL certificate for "<<name<<"\n";
        }
        return;
    } //else:

    auto paths = add_ssl_to_config(name); // let it throw.

    check_ssl_certificate(paths.crt, paths.key); // let it throw.

    try { install_cron_job(CRON_CHECK); }
    catch (...) {
        std::cerr<<"add_ssl_if_needed warning: cannot use cron to rebuild ";
        std::cerr<<"the self-signed SSL certificates.\n";
    }
}


void remove_cron_job(const Line & CRON_LINE, const std::string & name)
{
    static const char * filename = "/etc/crontabs/root";

    const auto const_conf = read_file(filename);

    bool changed = false;
    auto conf = std::string{};

    size_t prev = 0;
    size_t curr = 0;
    while ((curr=const_conf.find('\n', prev)) != std::string::npos) {

        auto line = const_conf.substr(prev, curr-prev+1);

        if (line==delete_if(line, CRON_LINE.RGX(), name)) {
            conf += line;
        } else { changed = true; }

        prev = curr + 1;
    }

    if (changed) {
        write_file(filename, conf);

        std::cerr<<"Do not rebuild the self-signed SSL certificate";
        std::cerr<<(name.empty() ? std::string{"s"} : " for '"+name+"'");
        std::cerr<<" annually with cron anymore."<<std::endl;

#ifndef NO_UBUS
        if (ubus::call("service", "list", UBUS_TIMEOUT).filter("cron"))
        { call("/etc/init.d/cron", "reload"); }
#endif
    }
}


inline void del_ssl_directives_from(const std::string & name,
                                    const bool isdefault)
{
    const std::string prefix = std::string{CONF_DIR} + name;

    std::string conf = read_file(prefix+".conf");

    const std::string & const_conf = conf; // iteration needs const string.
    rgx::smatch match; // captures str(1)=indentation spaces, str(2)=server name
    for (auto pos = const_conf.begin();
        rgx::regex_search(pos, const_conf.end(), match, NGX_SERVER_NAME.RGX());
        pos += match.position(0) + match.length(0))
    {
        if (match.str(2).find(name) == std::string::npos) { continue; }

        const std::string indent = match.str(1);

        std::string adds = isdefault ?
            get_if_missed(conf, NGX_INCLUDE_LAN_LISTEN_DEFAULT,"",indent) :
            get_if_missed(conf, NGX_INCLUDE_LAN_LISTEN, "", indent);

        if (adds.length() > 0) {
            pos += match.position(1);

            conf = std::string(const_conf.begin(), pos) + adds
                    + std::string(pos, const_conf.end());

            conf = isdefault ?
                delete_if(conf, NGX_INCLUDE_LAN_SSL_LISTEN_DEFAULT.RGX())
                : delete_if(conf, NGX_INCLUDE_LAN_SSL_LISTEN.RGX());

            const auto crtpath = prefix+".crt";
            conf = delete_if(conf, NGX_SSL_CRT.RGX(), crtpath);

            const auto keypath = prefix+".key";
            conf = delete_if(conf, NGX_SSL_KEY.RGX(), keypath);

            conf = delete_if(conf, NGX_SSL_SESSION_CACHE.RGX());

            conf = delete_if(conf, NGX_SSL_SESSION_TIMEOUT.RGX());

            write_file(prefix+".conf", conf);

            std::cerr<<"Deleted SSL directives from "<<prefix<<".conf\n";
        }

        return;
    }

    auto errmsg = std::string{"del_ssl_directives_from error: "};
    errmsg += "cannot delete SSL directives from " + name + ".conf, missing: ";
    errmsg += NGX_SERVER_NAME.STR(name, "\n    ") + "\n";
    throw std::runtime_error(errmsg.c_str());
}


inline auto del_ssl_from_config(const std::string & name)
{
    auto sec = uci::package{"nginx"}[name]; // let it throw.

    struct { std::string crt; std::string key; } ret;

    auto manage = false;
    for (auto opt : sec) {

        if (opt.name()=="ssl_session_cache") { opt.del(); continue; } //else:

        if (opt.name()=="ssl_session_timeout") { opt.del(); continue; } //else:

        for (auto itm : opt) {

            if (opt.name()=="ssl_certificate") {
                ret.crt = itm.name();
                opt.del();
                break;
            } //else:

            if (opt.name()=="ssl_certificate_key") {
                ret.key = itm.name();
                opt.del();
                break;
            } //else:

            if (opt.name()==MANAGE_SSL && itm) {
                manage = true;
                opt.del();
                break;
            } //else:

            if (opt.name()=="listen" || opt.name()==LISTEN_LOCALLY) {
                static const auto rgx_port = rgx::regex
                    {R"(^\s*([^:]*:|\[[^]]*\]:)?443(\s.*)?\sssl(\s|$|;))"};
                auto val = regex_replace(itm.name(), rgx_port, "$0180$2$3");
                itm.rename(val.c_str());
                continue;
            }
        }
    }
    if (manage) {
        sec.commit();
        return ret;
    } //else:

    auto errmsg = std::string{"del_ssl error: not changing the server config "};
    errmsg += "without: uci set nginx."+name+"."+MANAGE_SSL.data()+"=true ";
    throw std::runtime_error(errmsg);
}


auto del_ssl_legacy(const std::string & name) -> bool
{
    const auto legacypath = std::string{CONF_DIR} + name + ".conf";

    if (access(legacypath.c_str(), R_OK)!=0) { return false; }

    try { remove_cron_job(CRON_CMD, name); }
    catch (...) {
        std::cerr<<"del_ssl warning: cannot remove cron job rebuilding ";
        std::cerr<<"the self-signed SSL certificate for "<<name<<"\n";
    }

    try { del_ssl_directives_from(name, name==LAN_NAME); }
    catch (...) {
        std::cerr<<"del_ssl error: ";
        std::cerr<<"cannot delete SSL directives from "<<name<<".conf\n";
        throw;
    }

    return true;
}


void del_ssl(const std::string & name)
{
    auto crtpath = std::string{};
    auto keypath = std::string{};

    if (del_ssl_legacy(name)) { //let it throw.
        crtpath = std::string{CONF_DIR} + name + ".crt";
        keypath = std::string{CONF_DIR} + name + ".key";
    }

    else {
        auto paths = del_ssl_from_config(name); //let it throw.
        crtpath = paths.crt;
        keypath = paths.key;
    }

    if (remove(crtpath.c_str())!=0) {
        auto errmsg = "del_ssl warning: cannot remove "+crtpath;
        perror(errmsg.c_str());
    }

    if (remove(keypath.c_str())!=0) {
        auto errmsg = "del_ssl warning: cannot remove "+keypath;
        perror(errmsg.c_str());
    }
}


void check_ssl(const uci::package & pkg)
{
    auto at_least_one_has_manage_ssl = false;

    for (auto sec : pkg) {
        if (sec.anonymous() || sec.type()!="server") { continue; } //else:

        const auto legacypath = std::string{CONF_DIR}+sec.name()+".conf";
        if (access(legacypath.c_str(), R_OK)==0) { continue; } //else:

        auto keypath = std::string{};
        auto crtpath = std::string{};
        auto manage = false;

        for (auto opt : sec) {
            for (auto itm : opt) {
                if (opt.name()=="ssl_certificate_key") { keypath = itm.name(); }
                else if (opt.name()=="ssl_certificate") { crtpath = itm.name();}
                else if (opt.name()==MANAGE_SSL && itm) { manage = true; }
            }
        }

        if (manage && !crtpath.empty() && !keypath.empty()) {
            at_least_one_has_manage_ssl = true;

            try { check_ssl_certificate(crtpath, keypath); }
            catch (...) {
                std::cerr<<"check_ssl warning: cannot build certificate '";
                std::cerr<<crtpath<<"' or key '"<<keypath<<"'.\n";
            }
        }
    }

    auto suffix = std::string_view
        {" cron job checking the self-signed SSL certificates.\n"};

    if (at_least_one_has_manage_ssl) {
        try { install_cron_job(CRON_CHECK); }
        catch (...) { std::cerr<<"check_ssl warning: cannot install"<<suffix; }
    }

    else if(access("/etc/crontabs/root", R_OK) == 0) {
        try { remove_cron_job(CRON_CHECK); }
        catch (...) { std::cerr<<"check_ssl warning: cannot remove"<<suffix; }
    } //else: do nothing
}


#endif
