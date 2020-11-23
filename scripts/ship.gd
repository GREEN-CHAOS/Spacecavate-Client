extends Spatial

var pos
var timeelapsed = 0
var futurepositions : Dictionary = {}
#  example element [t,postion]
#  					t in ms
var passedms = 0

func _ready():
	pos = Vector3(0,0,0)
	for i in range(10000):
		futurepositions[i] = pos
		pos += Vector3(0.01,0.01,0.01)
	pass # Replace with function body.



func _process(delta):
	timeelapsed += delta
	set("translation",futurepositions[int(timeelapsed*5)])
#	passedms += delta
#	var newpos = futurepositions.get(passedms)
#	print(newpos)
