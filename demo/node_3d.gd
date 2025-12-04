extends Node3D

@onready var mesh_instance_3d: MeshInstance3D = $MeshInstance3D
@onready var audio_stream_player_3d: AudioStreamPlayer3D = $AudioStreamPlayer3D

var retro_player: RetroPlayer = RetroPlayer.new()

var core_path: String = "C:\\roms\\cores\\"
var core_name: String = "mame2003_plus_libretro.dll"
## fbneo_libretro.dll
## dosbox_pure_libretro
## vice_x64sc_libretro

var content_path: String = "C:\\roms\\"
var content_name: String = "arcade\\defender.zip"
#ddragon.zip
#defender.zip
#digdug.zip"

var config_path = ProjectSettings.globalize_path("res://assett/mame2003plus.cfg").replace("/", "\\")

func _ready():
		## Connect the joypad attached/detatched signal
	Input.joy_connection_changed.connect(initialize_controllers)
	
	## initialize the attached controllers
	var connected = Input.get_connected_joypads()
	if connected.size() > 0:
		for id in connected:
			initialize_controllers(id, true)
	
	retro_player.set_render_surface(mesh_instance_3d)
	retro_player.set_audio_player(audio_stream_player_3d)
	retro_player.player_init([config_path], core_path + core_name, content_path + content_name, 2) 


func _process(_delta):
	if Input.is_action_just_pressed("quit_game"):
		retro_player.quit()
		get_tree().quit()
	else:
		retro_player.run()
		pass


func initialize_controllers(id: int, connected: bool) -> void:
	if connected:
		var event = {
			"event": "InitializeControllerEvent",
			"device" : id,
		   	"joypad_name" : Input.get_joy_name(id)
		}
		retro_player.input(event)
	else:
		## need to add deinit of joypad
		pass

func _input(event: InputEvent) ->void:
	if event is InputEventScreenTouch:
		return
	
	if event is InputEventJoypadButton:
		var event_joypad: InputEventJoypadButton = event
		var joypad_event = {"event" : "InputEventJoypadButton",
							"device" :event_joypad.device,
							"button_index" : event_joypad.button_index,
							"pressed" : event_joypad.pressed}
		retro_player.input(joypad_event)
		return
	
	if event is InputEventJoypadMotion:
		return
	
	if event is InputEventAction:
		return
	
	retro_player.forward_input(event)
