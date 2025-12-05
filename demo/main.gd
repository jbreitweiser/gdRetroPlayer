extends MarginContainer

@onready var texture_rect: TextureRect = $TextureRect
@onready var audio_stream_player_2d: AudioStreamPlayer2D = $AudioStreamPlayer2D

var retro_player: RetroPlayer = RetroPlayer.new()

var core_path: String = "C:\\roms\\cores\\"
var core_name: String = "fbneo_libretro.dll"
## mame2003_plus_libretro.dll
## fbneo_libretro.dll
## vitaquake3_libretro.dll
## dosbox_pure_libretro
## vice_x64sc_libretro

var content_path: String = "C:\\roms\\"
var content_name: String = "arcade\\digdug.zip"
#ddragon.zip
#defender.zip
#digdug.zip"

var config_path = ProjectSettings.globalize_path("res://assett/mame2003plus.cfg").replace("/", "\\")
var joy_pad_map = {}

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
	retro_player.set_render_surface(texture_rect)
	retro_player.set_audio_player(audio_stream_player_2d)
	retro_player.player_init([config_path], core_path + core_name, content_path + content_name, 2) 

func initialize_controllers(id: int, connected: bool) -> void:
	if connected:
		var event = {
			"event": "InitializeControllerEvent",
			"device" : id,
		   	"joypad_name" : Input.get_joy_name(id)
		}
		retro_player.input(event)
		var data: Dictionary = load_json_file("res://assett/retropad_map.json")
		var joy_name = Input.get_joy_name(id)
		var joy_pad_key = "default"
		for key in data.keys():
			if joy_name.begins_with(key):
				joy_pad_key = key 
		
		joy_pad_map = data.get(joy_pad_key)
	else:
		## need to add deinit of joypad
		pass

func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("quit_game"):
		retro_player.quit()
		get_tree().quit()
	else:
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
		var pad_button: Dictionary = joy_pad_map.get(str(event_joypad.button_index))
		var retro_button = pad_button.get("retro_value")
		print("joypad:", pad_button.get("description"), " Retropad: ", pad_button.get("retro_description"))
		var joypad_event = {"event" : "InputEventJoypadButton",
							"device" :event_joypad.device,
							"button_index" : retro_button,
							"pressed" : event_joypad.pressed}
		retro_player.input(joypad_event)
		return
	
	if event is InputEventJoypadMotion:
		return
	
	if event is InputEventAction:
		return
	
	retro_player.forward_input(event)

func load_json_file(file_path: String):
	if not FileAccess.file_exists(file_path):
		print("Error: File not found at ", file_path)
		return {}

	var file: FileAccess = FileAccess.open(file_path, FileAccess.READ)
	if FileAccess.get_open_error() != OK:
		print("Error opening file: ", FileAccess.get_open_error())
		return {}

	var content: String = file.get_as_text()
	file.close()

	var json = JSON.new()
	if json.parse(content) != OK:
		print("JSON Parse Error: ", json.get_error_message(), " at line ", json.get_error_line())
		return {}

	if typeof(json.data) == TYPE_DICTIONARY:
		return json.data
	else:
		print("Data not in expected format: ")
		print(json.data)
		return {}
	
