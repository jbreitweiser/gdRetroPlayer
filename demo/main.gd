extends Node2D

@onready var texture_rect: TextureRect = $BoxContainer/TextureRect

var retro_player: RetroPlayer = RetroPlayer.new()
@onready var sprite_2d: Sprite2D = $Sprite2D

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	Input.joy_connection_changed.connect(initialize_controllers)
	
	var connected = Input.get_connected_joypads()
	if connected.size() > 0:
		for id in connected:
			initialize_controllers(id, true)
			
	retro_player.player_init([], "C:\\msys64\\home\\jbrei\\RetroPlayer\\demo\\assett\\cores\\fbneo_libretro.dll", "C:\\msys64\\home\\jbrei\\RetroPlayer\\demo\\assett\\cores\\defender.zip", 0)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("quit_game"):
		retro_player.quit()
		get_tree().quit()
	else:
		run_frame()
	

func run_frame() -> void:
	retro_player.run()
	var buffer: Image = retro_player.get_frame_buffer()
	var tex = ImageTexture.create_from_image(buffer)
	texture_rect.texture = tex

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
