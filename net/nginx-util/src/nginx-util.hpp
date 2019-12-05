#ifndef __NGINX_UTIL_H
#define __NGINX_UTIL_H

#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstring>

using namespace std;

static const auto PROGRAM_NAME = string{"/usr/bin/nginx-utils"};

static const auto CONF_DIR = string{"/etc/nginx/conf.d/"};

static const auto LAN_NAME = string{"_lan"};

static const auto LAN_LISTEN = string{"/var/lib/nginx/lan.listen"};

// mode: optional ios::binary and/or ios::app (default ios::trunc)
void write_file(const std::string & name, const std::string str,
                const std::ios_base::openmode mode=std::ios::trunc);

// mode: optional ios::binary (internally ios::ate|ios::in)
std::string read_file(const std::string & name,
                      const std::ios_base::openmode mode=std::ios::in);

// all S must be convertible to const char[]
template<typename ...S>
pid_t call(const char program[], S... args);

void create_lan_listen();

void init_lan();

void get_env(const string & name=LAN_NAME);



// ------------------------- implementation: ----------------------------------


void write_file(const std::string & name, const std::string str,
                const std::ios_base::openmode flag)
{
    std::ofstream file(name, flag);
    if (!file.good()) {
        throw std::ofstream::failure("write_file error: cannot open " + name);
    }

    file<<str<<std::flush;

    file.close();
}


std::string read_file(const std::string & name,
                      const std::ios_base::openmode mode)
{
    std::ifstream file(name, mode|std::ios::ate);
    if (!file.good()) {
        throw std::ifstream::failure("read_file error: cannot open " + name);
    }

    std::string ret{};
    const size_t size = file.tellg();
    ret.reserve(size);

    file.seekg(0);
    ret.assign((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

    file.close();
    return ret;
}


template<typename ...S>
pid_t call(const char program[], S... args)
{
    pid_t pid = fork();

    if (pid==0) { //child:
        execl(program, program, args..., (char*)NULL);
        _exit(EXIT_FAILURE);  // exec never returns.
    } else if (pid>0) { //parent:
        return pid;
    }

    std::string errmsg = "call error: cannot fork (";
    errmsg += std::to_string(errno) + "): " + std::strerror(errno);
    throw std::runtime_error(errmsg.c_str());
}


void create_lan_listen()
{
#ifdef openwrt
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

#endif
