extends Node

#Global Variables
var score:Array[int] = []
var play_state:bool = false
var play_count:int = 0
var play_speed:int = 8
var max_play_time:int = 256
var song_length:int = 32
var prev_play_note = null
var ticks_per_quarter_note = 480
var tempo_bpm = 120

var prev_tick = 9999999
