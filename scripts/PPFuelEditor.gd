extends WindowDialog


# Declare member variables here. Examples:
# var a = 2
# var b = "text"
# [][min,max,smallstep,bigstep]
var properties = [[0,100,2,5],[0,100,2,5],[0,100,2,5]]


# Called when the node enters the scene tree for the first time.
func _ready():
	popup_centered_ratio()			# for debugging only
	for i in properties.size():
		print(i)
		print(get_child(i+1).get_child(7).name)
		get_child(i+1).get_child(7).text = String(properties[i][0])
		get_child(i+1).get_child(8).text = String(properties[i][1])


# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass


func _on_Smaller0Top_pressed():
	pass # Replace with function body.
