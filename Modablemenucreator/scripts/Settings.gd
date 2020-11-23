extends Control

var yposfirst = 100
var yadditivesize = 100
var yposlast = 0
var Settingsarray = [] 
var VBOXarray
var ControlScroll
var maxheight = 0
var currentobjarray = []
var obj
var maxheightarray = []
export var isinmainmenu = false
export var isloaded = false
var lasterror = ""
var lasterrorstring
var Currentycontainer
var group
var Inputmethod = 1
var assignvalue
var waitingforinput = false
var currentproperty = ""
var currentevent
var errortranslation = ["Generic","Unavailable","Unconfigured","Unauthorized","Parameter range","Out of memory","File does not exist"]
var groupinstance = preload("res://Modablemenucreator/scenes/group.tscn")
var tabinstance = preload("res://Modablemenucreator/scenes/Tab.tscn")
var nodesavepath = "res://Modablemenucreator/scenes/TabContainersave.tscn"
var Settingsarrayscript = load("res://Modablemenucreator/scripts/Settingsarraysciptclear.gd")
signal newinputorclosed
var Settingsarraynode



func _unhandled_input(event):
	print("INPUT")
	print(waitingforinput)
	print(assignvalue)
	if waitingforinput:
		assignvalue = event.as_text()
		print("unhandeled")
		if InputMap.action_has_event(currentproperty,event):
			InputMap.action_erase_event(currentproperty,event)
		InputMap.action_add_event(currentproperty,event)
		print(event.device)
		InputMap.action_add_event(currentproperty,event)
		waitingforinput = false
		emit_signal("newinputorclosed")
	
	pass
func _ready():
	#vboxcontainerlist    [x][properties[]]
	$Mainmenu.set_visible(isinmainmenu)           # comment it in later after testing
	$TabContainer.set_visible(!isinmainmenu)      # comment it in later after testing
	Settingsarraynode = Node.new()
	Settingsarraynode.set("name","Settingsarray")
	Settingsarraynode.set_script(load("res://Modablemenucreator/scripts/Settingsarrayscipt.gd"))
	add_child(Settingsarraynode)
	print(String(OS.get_time()))

	Settingsarray = get_node("Settingsarray").Settingsarray
	VBOXarray = [[["name","ExtraTab"],["anchor_right",1],["anchor_left",0.5]]]
	setup_keymapping_columns(Settingsarray)
	print(String(OS.get_time()))





func _on_Settings_pressed(settomainmenu = false, err = ""):
	if err != "":
		lasterror = err
		print("erroronsettings:"+err)
		print(lasterror)
	if settomainmenu:
		$TabContainer.set_visible(false)
		$Mainmenu.set_visible(false)
	elif isloaded:
		$errorwarninglabel.text = ""
		$TabContainer.set_visible(!isinmainmenu)
		$Mainmenu.set_visible(isinmainmenu)
		isinmainmenu = !isinmainmenu
	else:
		$TabContainer.set_visible(false)
		$Mainmenu.set_visible(true)
		if errortranslation[int(lasterror)-1] != "":
			lasterrorstring = errortranslation[int(lasterror)]
			$errorwarninglabel.text = "error while loading Settings Configfile try restarting or try to find the config file you may have moved it. "+lasterrorstring
		else:
			$errorwarninglabel.text = "error while loading Settings Configfile try restarting or try to find the config file you may have moved it. Error Code: " + lasterror


func on_change(value, button):
	
	var currentycontainersent = button.get_parent()
	var steporfixed = button.name.split("valstep")[1].rsplit("ifpassedorsteporfixed")[0]
	var usepassedvalue_useasstep_useasfixed = int(button.name.split("ifpassedorsteporfixed")[1])
	
	var Isinputevent = currentycontainersent.name.split("InputMaporproperty")[1].rsplit("popup")[0]
	var popup = currentycontainersent.name.split("popup")[1].rsplit("property(")[0]
	print(popup)
	var currproperty = currentycontainersent.name.split("property(")[1].rsplit(")group")[0]
	var currentvalue = _loadproperty(currproperty)

	if Isinputevent:
		if !InputMap.has_action(currproperty):
			InputMap.add_action(currproperty)
		currentproperty = currproperty
		waitingforinput = true
	else:
		if usepassedvalue_useasstep_useasfixed == 0:
			# as step
			if value is float:
				assignvalue = float(value+float(steporfixed))
			elif value is int:
				assignvalue = int(value + int(steporfixed))
			elif value is bool:
				assignvalue = bool(value+bool(steporfixed))
		elif usepassedvalue_useasstep_useasfixed == -1:
			# use "value"
				assignvalue = value
		elif usepassedvalue_useasstep_useasfixed == 1:
			# use "steporfixedasvalue"
				assignvalue = steporfixed
			
	
	
	if bool(popup):
		$WindowDialog.popup_centered()
		$WindowDialog.window_title = currproperty
		if !Isinputevent: 
			$WindowDialog/LineEdit.text = "Succesfully changed to:"
			$WindowDialog/LineEdit.text += String(assignvalue)
		else:
			$WindowDialog/LineEdit.text = String(currentvalue) +"\n\n" + "Please assign new value by pressing an Key"
			
			yield(self,"newinputorclosed")
			print(assignvalue)
			$WindowDialog/LineEdit.text = String(assignvalue)
	
	currentproperty = currproperty
	Inputmethod = Isinputevent
	for i in currentycontainersent.get_child_count():
		if currentycontainersent.get_child(i).name.split("change")[1].rsplit("valstep")[0] == "True":
			currentycontainersent.get_child(i).text = String(assignvalue)
	_saveproperty(assignvalue,currproperty)



func setup_keymapping_columns(list):
	print(list)
	
	
# Creating the Settingsnodes

	# for each Tab
	for z in list.size():
	# for each Tab
		var Tab = tabinstance.instance()
		for property in list[z][1].size():
			print("set"+list[z][1][property][0]+"    TO    "+String(list[z][1][property][1]))	
			Tab.set(list[z][1][property][0],list[z][1][property][1])
			print("tabexists NOT")
		ControlScroll = Tab.get_child(0).get_node("Control")
		print("ControllScroll : ",ControlScroll.name)
		Tab.get_child(0).get_child(0).set("rect_min_size",Vector2(OS.get_real_window_size().x,40000))
		# for each "line"
		$TabContainer.add_child(Tab)
		for y in list[z][0].size():
			print("y:"+String(y))
			var ylist = list[z][0][y][0]
			Currentycontainer = HBoxContainer.new()
			Currentycontainer.name = "y"+String(y)+"InputMaporproperty"+String(list[z][0][y][3])+"popup"+String(list[z][0][y][4])+"property("+String(list[z][0][y][2])+")group"+String(list[z][0][y][1])
			Currentycontainer.set("anchor_right",1)
			Currentycontainer.set_alignment(BoxContainer.ALIGN_CENTER)
			ControlScroll.add_child(Currentycontainer)
			for x in ylist.size():
			#for each column
				print(ylist[x])
				obj = ClassDB.instance(ylist[x][0])
				print("test")
				var propertylist = ylist[x][2]
				for property in propertylist.size():
				#for each property
					obj.set(propertylist[property][0],propertylist[property][1])
					
				obj.name = "y"+String(y)+"x"+String(x)+"change"+String(ylist[x][3])+"valstep"+String(ylist[x][4])+"ifpassedorsteporfixed"+String(ylist[x][5])
				Currentycontainer.add_child(obj)
				
				if obj is Button:
					print("connected")
					obj.toggle_mode = true
					obj.connect("toggled",self,"on_change",[obj])
				elif obj is Slider or obj is SpinBox:
					obj.connect("value_changed",self,"on_change",[obj])
				elif obj is LineEdit:
					obj.connect("text_entered",self,"on_change",[obj])
				else:
					print("notconnected")
			if list[z][0][y][1] != null && obj != null && Currentycontainer != null:
				ControlScroll.remove_child(Currentycontainer)
				var group
				if not ControlScroll.has_node(String(list[z][0][y][1])):
					
					group = groupinstance.instance()
					ControlScroll.add_child(group)
					group.name = String(list[z][0][y][1])
					var grouptopControl = group.get_child(0)
					var grouptoextendControl = group.get_child(1)
					var groupbutton = grouptopControl.get_child(0)
					var grouplabel = grouptopControl.get_child(1)
					
					
					group.rect_min_size.y = grouptoextendControl.rect_min_size.y+grouptopControl.rect_min_size.y
					
					
					grouplabel.text = list[z][0][y][1]
					print("groupregistered:  "+String(group.name))
					
					groupbutton.connect("toggled",self,"group_button_toggled",[group])
					grouptopControl.rect_min_size.y = max(grouplabel.rect_min_size.y,groupbutton.rect_min_size.y)
					
				else:
					group = ControlScroll.get_node(list[z][0][y][1])
				Currentycontainer.margin_left = group.get_node("grouptop/groupbutton").rect_size.y
				group = group.get_node("groupbottom/ExtensionContainer")
				group.add_child(Currentycontainer)
				group.rect_min_size.y += Currentycontainer.rect_size.y
				Currentycontainer.alignment = BoxContainer.ALIGN_BEGIN
				Currentycontainer.margin_left = 380
			#for each y
			
			for groupnum in ControlScroll.get_child_count():
				for ycontainer in ControlScroll.get_child(groupnum).get_child(1).get_child(0).get_child_count():
					var prop = Properties.get(ControlScroll.get_child(groupnum).get_child(1).get_child(0).get_child(ycontainer).name.split("property(")[1].rsplit(")group")[0])
					for xnode in ControlScroll.get_child(groupnum).get_child(1).get_child(0).get_child(ycontainer).get_child_count():
						if ControlScroll.get_child(groupnum).get_child(1).get_child(0).get_child(ycontainer).get_child(xnode).name.split("change")[1].rsplit("valstep")[0] == "True":
							ControlScroll.get_child(groupnum).get_child(1).get_child(0).get_child(ycontainer).get_child(xnode).text = String(prop)
	isloaded = true
	var packedscene = PackedScene.new()
	print("savedScene")
	get_node("TabContainer").print_tree_pretty()
	set_childrens_owners_to($TabContainer)
	print_tree_owners($TabContainer)
	packedscene.pack(get_node("TabContainer"))
	print(packedscene)
	if  OK != ResourceSaver.save(nodesavepath,packedscene):
		print("failed to save")
	else:
		print("savedscene")
		Settingsarraynode.Settingsarray = []
		Settingsarrayscript.take_over_path("res://Modablemenucreator/scripts/Settingsarrayscipt.gd")
		

func print_tree_owners(node:Node):
	print(node.name, " : " ,node.owner)
	for i in node.get_child_count():
		print_tree_owners(node.get_child(i))

func set_childrens_owners_to(targetOwner : Node):
	targetOwner.set_owner($TabContainer)
	for i in targetOwner.get_child_count():
		set_childrens_owners_to(targetOwner.get_child(i))

func group_button_toggled(down,buttongroup):
	print(buttongroup.name+"button_down:"+String(down))
	var extension = buttongroup.get_node("groupbottom")
#
#	var animation = Animation.new()
#	animation needs to be added later !!
#
#	var index = buttongroup.get_index()
#	var nextgroup = buttongroup.get_child(index+1)
#	var trackid = animation.add_track(Animation.TYPE_VALUE)
#	animation.track_set_path(trackid,nextgroup.name+":rect_position.y")
#	animation.track_insert_key(trackid,0,nextgroup.rect_position.y)
#	animation.track_set_interpolation_type(trackid,Animation.INTERPOLATION_LINEAR)
#	animation.track_insert_key(trackid,1,buttongroup.rect_position.y+extension.rect_min_size.y)
#	get_node("AnimationPlayer").add_animation("Animation",animation)
#	get_node("AnimationPlayer").play("Animation")
	var top = buttongroup.get_node("grouptop")
	
	if down:
		buttongroup.rect_size.y = top.rect_size.y+extension.rect_size.y
	else:
		buttongroup.rect_size.y = top.rect_size.y
	extension.visible = down


func _loadproperty(propertytoload):
	if Properties.get(propertytoload) != null:
		return Properties.get(propertytoload)
	else:
		print(ERR_CANT_ACQUIRE_RESOURCE)
	
func _saveproperty(newvalue,propertytosave):
	if Properties.get(propertytosave) != null:
		Properties.set(propertytosave,newvalue)
	else:
		print("property not added to Singleton:",String(propertytosave))


func _on_WindowDialog_popup_hide():
	emit_signal("newinputorclosed")


