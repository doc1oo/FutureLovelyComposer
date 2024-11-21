extends Node2D

var thread = Thread.new()
var app_running = true    # スレッド制御用フラグ
var gds_obj 
var frame_count = 0

var mutex

var gdtune
const PATH_SAVEDATA = "user://save_data.txt"

var game_data = {
  "name": "hero", # プレイヤー名
  "hp": 10 # HP
}

var frequency = 440
#call_deferred("add_child", gdtune)
var thread_count = 0


func _ready():
	#gdtune = GDTune.new()
	#add_child(gdtune)
	print("Godot Current Directory: " + OS.get_executable_path().get_base_dir())

	var resource_root_path = ProjectSettings.globalize_path("res://")

	get_tree().set_auto_accept_quit(false)	
	
	mutex = Mutex.new()
	gdtune = GDTune.new()
	#gdtune.init("","")
	gdtune.init("C:/Program Files/Common Files/CLAP/","my_clap-saw-demo-imgui.clap")
	
	gdtune.param_change("Resonance", 1.0, 0, 0.0)
	#gdtune.param_change("Amplitude Attack (s)", 1.0, 0, 0.0)
	#gdtune.param_change("Amplitude Release (s)", 1.0, 0, 0.0)
	gdtune.param_change("Oscillator Detuning (in cents)", 0.0, 0, 0.0)
	gdtune.param_change("Filter Type",5.0, 0, 0.0)
	"""
		
	Plugin parameters ----------
	Unison Count ... current=3.0 default=3.0 min=1.0 max=7.0
	Unison Spread in Cents ... current=10.0 default=10.0 min=0.0 max=100.0
	Oscillator Detuning (in cents) ... current=0.0 default=0.0 min=-200.0 max=200.0
	Amplitude Attack (s) ... current=0.01 default=0.01 min=0.0 max=1.0
	Amplitude Release (s) ... current=0.20000000000000001 default=0.20000000000000001 min=0.0 max=1.0
	Deactivate Amp Envelope ... current=0.0 default=0.0 min=0.0 max=1.0
	Pre Filter VCA ... current=1.0 default=1.0 min=0.0 max=1.0
	Cutoff in Keys ... current=69.0 default=69.0 min=1.0 max=127.0
	Resonance ... current=0.69999999999999996 default=0.69999999999999996 min=0.0 max=1.0
	Filter Type ... current=0.0 default=0.0 min=0.0 max=5.0
	"""
	
	#add_child(gdtune)
	gdtune.play_note(64, 0.2, 100, 0, 0.0)
	gdtune.play_note(66, 0.2, 100, 0, 0.1)
	gdtune.play_note(68, 0.2, 100, 0, 0.2)
	gdtune.play_note(69, 0.2, 100, 0, 0.3)
	gdtune.play_note(71, 0.2, 100, 0, 0.4)
	gdtune.play_note(76, 1.5, 100, 0, 0.7)
	
	print("gdtune.get_instruments_info()")
	var godot_json_str = gdtune.get_instruments_info()
	
	var json = JSON.new()
	var result = json.parse(godot_json_str)
	var plug_info: Dictionary
	if result == OK:
		var data_received = json.data
		plug_info = json.get_data()
		if typeof(plug_info) == TYPE_DICTIONARY:
			print(plug_info) 
		else:
			print("Unexpected data")
	else:
		pass#print("JSON Parse Error: ", json.get_error_message(), " in ", json_string, " at line ", json.get_error_line())

	print("file: ", plug_info["file"])
	
	var params_json_str = gdtune.get_loaded_plugin_params_json()
	#print("params_json_str: ", params_json_str)
	var params_info: Dictionary
	result = json.parse(params_json_str)
	if result == OK:
		params_info = json.get_data()
		if typeof(params_info) == TYPE_DICTIONARY:
			print(params_info) 
		else:
			print("Unexpected data")
	
	print(params_info)

	var label_settings = LabelSettings.new()
	label_settings.font_size = 20
	
	var base_pos_x = 40 
	var base_pos_y = 100 

	var plugin_name_label = Label.new()
	plugin_name_label.text = plug_info["plugin_descriptor"]["name"] + " / " + plug_info["plugin_descriptor"]["vendor"]
	plugin_name_label.set_position( Vector2(base_pos_x, 50) )
	plugin_name_label.label_settings = label_settings
	add_child(plugin_name_label)

	for i in range(params_info["param-count"]):
		var prm = params_info["param-info"][i]
		print(prm["name"]);
		print(prm["id"]);
		#print(prm["values"]);
		print(prm["values"]["current"]);
		print(prm["values"]["default"]);
		print(prm["values"]["min"]);
		print(prm["values"]["max"]);
		var slider = HSlider.new()
		slider.set_position(Vector2(base_pos_x+300, i*30+base_pos_y))
		slider.min_value = float(prm["values"]["min"])
		slider.max_value = float(prm["values"]["max"])
		slider.value = prm["values"]["current"]
		slider.editable = true
		slider.scrollable = true
		slider.step = 0.01
		slider.size = Vector2(200, 30)
		slider.visible = true
		slider.top_level = true
		slider.set_meta("param_name", prm["name"])
		slider.set_meta("param_id", prm["id"])
		slider.value_changed.connect(_on_h_slider_value_changed.bind(slider))
		#slider. = prm["values"]["current"]
		add_child(slider)
		var label = Label.new()
		label.text = prm["name"]
		label.set_position( Vector2(base_pos_x, i*30+base_pos_y) )
		label.label_settings = label_settings
		add_child(label)
		var value_label = Label.new()
		value_label.name = "plugin_params_value_label_" +  str(prm["id"])
		value_label.text = str(prm["values"]["current"])
		value_label.set_position( Vector2(base_pos_x+600, i*30+base_pos_y) )
		value_label.label_settings = label_settings
		add_child(value_label)
	
	thread.start(audio_thread, Thread.PRIORITY_HIGH)
	pass


func  _process(delta: float) -> void:
	var m_pos = get_global_mouse_position()
	var lm_pos =  get_local_mouse_position()
	frequency = 1000 - m_pos.y
	
	frame_count+=1
	#$GDTune.set_frequency(frequency)			# pass parameters to GDExtension GDTune
	
	var t = lm_pos.y / 1000.0 

	#gdtune.param_change("Resonance", 0.5*sin(t)*0.5, 0, 0.0)
	
	queue_redraw()

func _input(event):
	if event is InputEventMouseButton:
		if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
			if gdtune:
				var m_pos = get_global_mouse_position()
				frequency = 1000 - m_pos.y
				var note_key = frequency / 50 + 60
				print("play_note:" + str(note_key))
				gdtune.play_note(note_key, 1.0, 100, 0, 0.0)
		elif Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
			print("右クリックされました")
			
			
func _draw() -> void:
	# Drawing Sine Wave ----------------------
	var color = Color(0.5, 1.0, 0.8, 0.6)
	var array = PackedVector2Array()
	for i in range(1000):
		var y = 300*sin(i*2*PI/50000.0*frequency) + 320
		var vec = Vector2(i, y)
		array.append(vec)

	draw_polyline(array, color)

# もしゲームが終了(ウィンドウの右上のXを押下したか、Alt+F4など)で終了した場合
func _notification(what: int) -> void:
	#print("EV"+ var_to_str(what) )
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		# 終了リクエストを受け取った時の処理
		# ↓ここを好きな処理に置き換え
		quit_game()


# アプリ終了処理時に自動的に呼ばれるシステム関数（get_tree().quit()の後など）
func _exit_tree():
	print("main.gd: _exit_tree() start")
	app_running = false
	
	# Godotの別スレッドを終了
	if thread.is_alive():
		print("main.gd: thread.wait_to_finish() start")
		thread.wait_to_finish()
		print("main.gd: thread.wait_to_finish() finished")
	else:
		print("main.gd: thread is not alive.")

	# GDTuneを解放	
	if gdtune:
		print("main.gd: gdtune.queue_free() start")
		#gdtune.queue_free()  # または custom_node.free()
		gdtune.deinit()
		gdtune.free()
		print("main.gd: gdtune.queue_free() end")


func quit_game():
	print("main.gd: Quitting app start.")

	#game_data["player_name"] = $HUD/PlayerName.text
	#write_save_data()
	print("main.gd: get_tree().quit() start")
	get_tree().quit()		# 手動で終了処理をしないと閉じれない 
	print("main.gd: get_tree().quit() finished.")
	# _exit_tree() startがこの後呼ばれます


func on_button_down():
	print("main.gd: On Button Down!")	


func audio_thread():
	
	# データを準備
	while app_running:
		#mutex.lock()
		# var m_pos = get_global_mouse_position()
		#var m_pos = call_deferred("get_global_mouse_position")
		#frequency = 1000 - m_pos.y
		#$GDTune.set_frequency(frequency)			# pass parameters to GDExtension GDTune
		#$GDTune.call_deferred("set_frequency", frequency)
		#gdtune.call_deferred("set_frequency", thread_count)
		#gdtune.set_frequency(thread_count)
		gdtune.update(0.0)
		#mutex.unlock()
		#print("test_thread: ", thread_count)
		
		OS.delay_msec(5)
		#OS.delay_usec(0)
			
		# var play_index = int(Globalv.play_count / Globalv.ticks_per_quarter_note) % Globalv.song_length
		
		"""
		if Globalv.play_state == true:
				
			var channel = 0 # 9 and 25 are for percussions.
			
			if (Globalv.play_count % Globalv.ticks_per_quarter_note) == 0:	# ノートを演奏

				var key = 80 - Globalv.score[play_index%64]
				#if Globalv.prev_play_note != null:
				#	$FrontCase.keyb_note_off(Globalv.prev_play_note)
				#$FrontCase.keyb_note_on(key)
				Globalv.prev_play_note = key
				
				channel = 0 # 9 and 25 are for percussions.
				var dic = {	"channel": channel, 
							"key": key,
							"velocity": 127, 
							"program": Globalv.program, 
							"tempo": Globalv.base_tempo}
				#$FrontCase.visible = false
				gds_obj.set_note_on(dic)
			
			var beats_per_second = Globalv.tempo_bpm / 60
			var rate_per_second = Globalv.ticks_per_quarter_note * beats_per_second
			Globalv.play_count += int(rate_per_second * 0.01)		# BPM120で1秒間に2回480になるぺ～ス
			Globalv.play_count %= rate_per_second * Globalv.song_length

			Globalv.play_count+= 1
		"""

		# オーディオ再生時間とミックスにかかった時間を計算します。
		#var time = $"../AudioStreamPlayer".get_playback_position() + AudioServer.get_time_since_last_mix()
		# 出力の遅延時間を取得して補正します
		#time -= AudioServer.get_output_latency()
		#print(" AudioServer.get_output_latency(): ", AudioServer.get_output_latency(), "  ", $"../AudioStreamPlayer".get_playback_position(), "  ", AudioServer.get_time_since_last_mix(), "  ", time)

		#gds_obj._process(0.01)

		thread_count += 1
	
	print("audio_thread() end.")
	return 

	
	
	"""
	Globalv.score.resize(64)
	
func _ready():
	#gdtune = GDTune.new()
	#add_child(gdtune)

	mutex = Mutex.new()
	gdtune = GDTune.new()
	thread.start(audio_thread, Thread.PRIORITY_HIGH)
	pass

				
	print("MyGDSynthesizer初期化開始")
# これとか初期家系処理がないと No Free Toneエラーが出る
	
	
	print("MyGDSynthesizer初期化終了")
	
	var channel = 0 # 9 and 25 are for percussions.
	var dic = {	"channel": channel, 
				"key": 80,
				"velocity": 127, 
				"program": Globalv.program, 
				"tempo": Globalv.base_tempo}
	#$FrontCase.visible = false
	gds_obj.set_note_on(dic)
	#AudioServer
	var stream = gds_obj.get_stream_playback()
	print(stream)
	
	#$FrontCase/GDSynthesizer.set_note_on(dic)
	#$FrontCase.keyb_note_on(69)
	
	print("オーディオ生成スレッドを開始")
	
	
	
	get_tree().set_auto_accept_quit(false)		# 手動でアプリ終了処理をさせるようにする そうしないとWM_CLOSEが発生せずに終了してしまう場合がありデータがセーブできないまま終了する
	randomize()
	
# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if $AudioStreamPlayer.is_playing(): 
		var t_playback = $AudioStreamPlayer.get_stream_playback()
		var buf_samples = int(sample_hz * buffer_length)
		if t_playback.can_push_buffer(buf_samples/2): 
			print("feed! ", phase_g)
			var frames = PackedVector2Array()
			for i in range(0, buf_samples/2): 
				var sample = sin(phase_g)
				phase_g += increment_g
				frames.append(Vector2(sample, sample))
			t_playback.push_buffer(frames)
	"""


func _on_h_slider_value_changed(value: float, slider: HSlider) -> void:
	print("main.gd: _on_h_slider_value_changed(): ", value, " ", slider.get_meta("param_name"), " ",  slider.get_meta("param_id"))
	var param_id = slider.get_meta("param_id")
	get_node("plugin_params_value_label_" + str(param_id)).text = str(value)
	
	gdtune.param_change_by_id(param_id, value, 0, 0.0)
	pass # Replace with function body.
