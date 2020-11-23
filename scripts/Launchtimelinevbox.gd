extends VBoxContainer

onready var scene:PackedScene  = preload("res://scenes/Actionintimeline.tscn")

func _ready():
	addnewevent(0,"test","set",["throttle",100])
	addnewevent(-2,"test","set",["throttle",100])
	addnewevent(-1,"test","set",["throttle",100])
	addnewevent(-5,"test","set",["throttle",100])
	
	pass
	
func addnewevent(time : int ,eventname:String , event:String, arguments:Array = [null]):
	var obj = scene.instance()
	obj.name = String(time)
	var plusminus : String = ""
	if time >= 0:
		plusminus = "+"
	obj.get_node("Time").text += plusminus+String(time)+"s"
	obj.get_node("Eventname").text = eventname 
	add_child(obj)
	Servermanagement.sendtoserver(["addneweventtolaunchtimeline",eventname,event,arguments])
	sortchildren()
	

func deleteevent(time:int,eventname:String):
	Servermanagement.sendtoserver(["deleteeventfromlaunchtimeline",time,eventname])
	remove_child(get_node(String(time)))


func changeevent(time:int,eventname:String,propertytochange:String,newvalue):
	Servermanagement.sendtoserver(["changeeventfromlaunchtimeline",time,eventname,propertytochange,newvalue])
	var obj = get_node(String(time))
	if propertytochange == "Time":
		obj.get_child(0).text = newvalue
		sortchildren()
	elif propertytochange == "Eventname":
		obj.get_child(1).text =newvalue
	
func sortchildren():
	var array : Array = []
	for i in get_child_count():
		array.append(int(get_child(i).name))
	array.sort_custom(numbersorter,"sort_ascending")
	for i in array.size():
		move_child(get_node(String(array[i])),i)

class numbersorter:
	static func sort_ascending(a,b):
		if a < b:
			return true
		return false
