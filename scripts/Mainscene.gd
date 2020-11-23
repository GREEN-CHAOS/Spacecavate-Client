extends Control


# Declare member variables here. Examples:
# var a = 2
# var b = "text"
func _ready():
	pass

# Called when the node enters the scene tree for the first time.
func _resized():
	$Launchtimelinecontrol/Launchtimeline/Launchtimelinescroll/Launchtimelinevbox.rect_min_size.x = $Launchtimelinecontrol/Launchtimeline/Launchtimelinescroll.rect_size.x
	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass



