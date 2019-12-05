#include "nginx-util.hpp"

int main(int argc, char * argv[])
{
#ifdef openwrt
cout<<"TODO: remove openwrt macro!"<<endl;
#endif

    //TODO more?
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
    else if (argc==3 && argv[1]==cmds[1][0]) { add_ssl_if_needed(argv[2]); }
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

