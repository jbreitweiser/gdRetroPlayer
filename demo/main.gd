extends Node2D

@onready var texture_rect: TextureRect = $BoxContainer/TextureRect
@onready var audio_stream_player_2d: AudioStreamPlayer2D = $BoxContainer/TextureRect/AudioStreamPlayer2D

var retro_player: RetroPlayer = RetroPlayer.new()

var audio_stream_generator: AudioStreamGenerator
var audio_playback: AudioStreamPlayback

var core_name: String = "mame2003_plus_libretro.dll"
## fbneo_libretro.dll
var core_path: String = "C:\\roms\\cores\\"

var content_name: String = "arcade - FB NEO\\ddragon.zip"
#digdug.zip"
var content_path: String = "C:\\roms\\"
# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	Input.joy_connection_changed.connect(initialize_controllers)
	
	var connected = Input.get_connected_joypads()
	if connected.size() > 0:
		for id in connected:
			initialize_controllers(id, true)
	
	retro_player.player_init([], core_path + core_name, content_path + content_name, 1)
	retro_player.set_texture_rect(texture_rect);

	#init_audio(retro_player.get_core_sample_rate())
	#audio_stream_player_2d.play()
	#audio_playback = audio_stream_player_2d.get_stream_playback()
	
	
func init_audio(sample_rate: float)-> void:
	audio_stream_generator = AudioStreamGenerator.new()
	audio_stream_generator.mix_rate = sample_rate
	## audio_stream_generator.mix_target = AudioStreamGenerator.MIX_TARGET_STEREO # Stereo output
	audio_stream_player_2d.stream = audio_stream_generator

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("quit_game"):
		retro_player.quit()
		get_tree().quit()
	else:
		run_frame()
	
	## Get audio samples and push to stream
	#var samples: PackedFloat32Array = retro_player.get_audio_samples()
	#@warning_ignore("integer_division")
	#var frame_count = samples.size() / 2
#
	## print("Audio samples=", samples, "  frame_count", frame_count)
	#if frame_count > 0:
		#var buffer = PackedVector2Array()
		#buffer.resize(frame_count)
		#for i in range(0, frame_count - 2, 2):  
			#var left_frame = samples[i*2]
			#var right_frame = samples[(i*2)+1]
			#buffer[i] = Vector2(left_frame, right_frame)
		#audio_playback.push_buffer(buffer)
		#retro_player.clear_audio_buffer()

func run_frame() -> void:
	retro_player.run()
	#var buffer: Image = retro_player.get_frame_buffer()
	#var tex = ImageTexture.create_from_image(buffer)
	#texture_rect.texture = tex

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
