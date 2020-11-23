#include "Common.h"
#include <Node.hpp>
#include <PacketPeerUDP.hpp>
#include <OS.hpp>
#include <IP.hpp>

class Servermanagement : public Node
{
    Ref<PacketPeerUDP> udp;
    Ref<PacketPeerUDP> udp2;
    String Currentiporhostnameserver;
    int8_t Currentportserver;
    String Currentpassword;
    bool connected;
    int sendingport;
    godot_signal recieveddata;
    GODOT_CLASS(Servermanagement, Node);
    String servercontactmessage;

    // Exposed properties

public:
    static const int defaultport = 70;
    static void _register_methods();
    void sendtoserver(Variant data);
    void _init();
    void _ready();
    void _process(float delta);
    void connecttoserver(String ip_address = "127.0.0.1", int port = Servermanagement::defaultport , String password = "");

private:
    void recieveddatamethode(Variant data);

    // add the methods here
};

