#include "nginx-util.hpp"


void create_lan_listen()
{
    string listen = "# This file is re-created if Nginx starts or"
                    " a LAN address changes.\n";
    string listen_default = listen;
    string ssl_listen = listen;
    string ssl_listen_default = listen;

    auto add_listen = [&listen, &listen_default,
                       &ssl_listen, &ssl_listen_default]
        (const string & prefix, string ip, const string & suffix) -> void
    {
        if (ip == "") { return; }
        ip = prefix + ip + suffix;
        listen += "\tlisten " + ip + ":80;\n";
        listen_default += "\tlisten " + ip + ":80 default_server;\n";
#ifdef NGINX_OPENSSL
        ssl_listen += "\tlisten " + ip + ":443 ssl;\n";
        ssl_listen_default += "\tlisten " + ip + ":443 ssl default_server;\n";
#endif
    };

    add_listen("", "127.0.0.1", "");
    add_listen("[", "::1", "]");

    auto lan_status = ubus::call("network.interface.lan", "status");

    for (auto ip : lan_status.filter("ipv4-address", "", "address")) {
        add_listen("",  blobmsg_get_string(ip), "");
    }

    for (auto ip : lan_status.filter("ipv6-address", "", "address")) {
        add_listen("[", blobmsg_get_string(ip), "]");
    }

    write_file(LAN_LISTEN, listen);
    write_file(LAN_LISTEN+".default", listen_default);
#ifdef NGINX_OPENSSL
    write_file(LAN_SSL_LISTEN, ssl_listen);
    write_file(LAN_SSL_LISTEN+".default", ssl_listen_default);
#endif
}


void init_lan()
{
#ifndef NGINX_OPENSSL
    create_lan_listen();
#else
    thread ubus{create_lan_listen};

    try { add_ssl_if_needed(LAN_NAME); }
    catch (...) {
        ubus.join();
        throw;
    }

    ubus.join();
#endif
}


void get_env(const string & name)
{
    const string prefix = CONF_DIR + name;
    cout<<"NGINX_CONF="<<'"'<<"/etc/nginx/nginx.conf"<<'"'<<endl;
    cout<<"CONF_DIR="<<'"'<<CONF_DIR<<'"'<<endl;
    cout<<"LAN_NAME="<<'"'<<LAN_NAME<<'"'<<endl;
    cout<<"LAN_LISTEN="<<'"'<<LAN_LISTEN<<'"'<<endl;
#ifdef NGINX_OPENSSL
    cout<<"LAN_SSL_LISTEN="<<'"'<<LAN_SSL_LISTEN<<'"'<<endl;
    cout<<"ADD_SSL_FCT="<<'"'<<ADD_SSL_FCT<<'"'<<endl;
    cout<<"NGX_SSL_CRT="<<'"'<<NGX_SSL_CRT.STR(prefix+".crt", "")<<'"'<<endl;
    cout<<"NGX_SSL_KEY="<<'"'<<NGX_SSL_KEY.STR(prefix+".key", "")<<'"'<<endl;
    cout<<"NGX_SSL_SESSION_CACHE="<<'"'<<
            NGX_SSL_SESSION_CACHE.STR("", "")<<
            NGX_SSL_SESSION_CACHE_PARAM(name)<<'"'<<endl;
    cout<<"NGX_SSL_SESSION_TIMEOUT="<<'"'<<
            NGX_SSL_SESSION_TIMEOUT.STR("", "")<<
            NGX_SSL_SESSION_TIMEOUT_PARAM<<'"'<<endl;
#endif
}


int main(int argc, char * argv[])
{
    string cmds[][2] = {
        { "init_lan", ""},
        { "get_env", " [name]"},
#ifdef NGINX_OPENSSL
        { ADD_SSL_FCT, " server_name" }
#endif
    };

    if (argc==2 && argv[1]==cmds[0][0]) { init_lan(); }

    else if (argc==2 && argv[1]==cmds[1][0]) { get_env(); }

    else if (argc==3 && argv[1]==cmds[1][0]) { get_env(argv[2]); }

#ifdef NGINX_OPENSSL
    else if (argc==3 && argv[1]==cmds[2][0]) { add_ssl_if_needed(argv[2]); }
#endif

    else {
        auto usage = string{"usage: "} + argv[0] + " [";

        for (auto cmd : cmds) { usage += cmd[0] + cmd[1] + "|"; }

        usage[usage.size()-1] = ']';
        cerr<<usage<<endl;
        return 2;
    }

    return 0;
}

