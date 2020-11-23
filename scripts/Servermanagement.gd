extends Node

var Currentiporhostnameserver
var Currentportserver
var Currentpassword : String
var udp := PacketPeerUDP.new()
var udp2 := PacketPeerUDP.new()
var connected := false
const defaultport = 70
var sendingport = 56
signal recievedddata(packet)



func connecttoserver(ip,port:int = defaultport,password:String = ""):
	udp.set_dest_address(ip,sendingport)
	var tosend = "REQUEST valid Spacecavate Server "+String(IP.get_local_addresses()[1])+"port "+String(defaultport)
	print(tosend)
	udp.put_packet(PoolByteArray(tosend.to_utf8()))
	print("connecting...")
	startlistening()
	print("startedlistening")
		
		
		
func startserver(port:int = defaultport,password:String = ""):
	pass
		
func startlistening(port:int = defaultport):
	print(port)
	var err = udp2.listen(defaultport)
	print(err)	
				

func sendtoserver(tosend):
	udp.put_packet(tosend)


func _process(delta):
	if !connected:
		pass
#		udp.put_packet("Hello World".to_utf8())
	if udp2.get_available_packet_count() > 0:
		connected = true
		emit_signal("recievedddata",udp2.get_packet())
		
		

func _ready():
		connect("recievedddata",self,"recieveddata")
		connecttoserver("127.0.0.1")
		connected = true


func recieveddata(packet: PoolByteArray):
	print("recieveddata:")
	print(packet.get_string_from_utf8()) 
	
	

