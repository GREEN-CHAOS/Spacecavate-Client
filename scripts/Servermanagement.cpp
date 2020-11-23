#include "Servermanagement.h"




void Servermanagement::_register_methods()
{
    register_method("_process", &Servermanagement::_process);
    register_method("_init", &Servermanagement::_init);
    register_method("_ready", &Servermanagement::_ready);
    register_method("recieveddatamethode", &Servermanagement::recieveddatamethode);
    register_method("connecttoserver",&Servermanagement::connecttoserver);
    register_method("sendtoserver",&Servermanagement::sendtoserver);

    register_signal<Servermanagement>((char *)"recieveddata","data", GODOT_VARIANT_TYPE_STRING);
}

void Servermanagement::_init()
{
    servercontactmessage = String("REQUEST valid Spacecavate Server ");
    connected = false;
    sendingport = 56;
    godot_signal recieveddata;
    Godot::print("Servermangementinitcalled");
    Godot::print("Servermangementinitcalled");
    connect("recieveddata",this,"recieveddatamethode");
}

void Servermanagement::_ready()
{
    udp = Ref<PacketPeerUDP>(PacketPeerUDP::_new());
    udp2 = Ref<PacketPeerUDP>(PacketPeerUDP::_new());
    String argument = "argument";
    udp -> put_var(argument);
    emit_signal("recieveddata",argument);
    Servermanagement::connecttoserver();
}

void Servermanagement::_process(float delta)
{
    if (udp2 -> get_available_packet_count() > 0){
        connected = true;
        emit_signal("recieveddata",udp2 -> get_packet());
    }
}

void Servermanagement::connecttoserver(godot::String ip_address, int port  , godot::String password ){
    udp -> set_dest_address(ip_address,port);
    udp -> put_var(servercontactmessage + "127.0.0.1");
    //udp -> put_var(ip.get_local_addresses());
    Godot::print("sendedrequest...");
    Error err = udp2 -> listen(port);
    //Godot::print(err);
    Godot::print("startedlistening");
    connected = true;
}


void Servermanagement::recieveddatamethode(Variant data)
{
    Godot::print(data);

}

void Servermanagement::sendtoserver(Variant tosend){
    udp -> put_var(tosend);
}