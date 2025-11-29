extends Node2D

@onready var texture_rect: TextureRect = $BoxContainer/TextureRect
@onready var audio_stream_player_2d: AudioStreamPlayer2D = $BoxContainer/TextureRect/AudioStreamPlayer2D

var retro_player: RetroPlayer = RetroPlayer.new()

var audio_stream_generator: AudioStreamGenerator
var audio_playback: AudioStreamPlayback

var core_name: String = "mame2003_plus_libretro.dll"
## fbneo_libretro.dll
var core_path: String = "C:\\roms\\cores\\"

var content_name: String = "arcade\\digdug.zip"
#ddragon.zip
#defender.zip
#digdug.zip"
var content_path: String = "C:\\roms\\"
# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	## Connect the joypad attached/detatched signal
	Input.joy_connection_changed.connect(initialize_controllers)
	
	## initialize the attached controllers
	var connected = Input.get_connected_joypads()
	if connected.size() > 0:
		for id in connected:
			initialize_controllers(id, true)
	
	## Initialize the core and selected content
	retro_player.player_init([], core_path + core_name, content_path + content_name, 2)
	retro_player.set_texture_rect(texture_rect);
	retro_player.set_audio_player(audio_stream_player_2d)

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

func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("quit_game"):
		retro_player.quit()
		get_tree().quit()
	else:
		run_frame()


func run_frame() -> void:
	retro_player.run()

##  The input method will accept the input event as a dictionary object
##  {"event" : Pass the event class name,
##   "device" : Device ID,
##   "button_index" : value of the setting.  Keycode, button mapping, axis, etc.,
##   "pressed" : true or false}
##
##  This dictionary approach allows the implementing frontend to map controls
##  and to capture input before forwarding it to the core.  You can also 
##  control when joypads are connected and disconnected and how they are 
##  initialized, even remaping them from one player to another.

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
