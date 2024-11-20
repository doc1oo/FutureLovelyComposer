# FutureLovelyComposer
 Prototype of the next generation Lovely Composer

It's a personal project for now. (work in progress)

![image](https://github.com/user-attachments/assets/d1613e6e-4de5-4771-b939-87570e299c42)

## 実行方法
godot_project/project.godotをGodot4.3で開いてエディタ上で実行します。

## コンパイル方法
* コマンドプロンプトでcd FutureLovelyComposerで移動したあと、./build.batを実行する
* 詳細なコンパイル方法の指定はCMake(CMakelist.txt)で

## 説明
今のところGodot側のプロジェクト（GUI中心/GDスクリプト）と、C++で書かれたGodotの拡張機能（GDTuneと呼ぶ自作オーディオシステムのGDExtension）に大きく分かれています。
* GDTuneのソースコードはgdtune/とlibs/以下
* Godot側のソースコードはgodot_project/以下
* 
## Dev tools
* Windows 11 
* Godot 4.3
* Visual Studio Community 2022 (『C++によるデスクトップ開発』をインストール) 17.11
* VSCode
* CMake

## License
一部のソースコードはclap-info、clap-hostを参考に書いており、miniaudioに関する一部のコードは公式サイトのチュートリアルから引用しています。

使用しているライブラリ等についてはそれぞれに定められたライセンスになります。（licenses/を参照のこと）
* [clap](https://github.com/free-audio/clap)
* [clap-info](https://github.com/free-audio/clap-info)
* [clap-helper](https://github.com/free-audio/clap-helpers)
* [clap-host](https://github.com/free-audio/clap-host)
* [clap-saw-demo-imgui](https://github.com/free-audio/clap-saw-demo-imgui)
* [godot](https://github.com/godotengine/godot)
* [godot-cpp](https://github.com/godotengine/godot-cpp) 
* [jsoncpp](https://github.com/open-source-parsers/jsoncpp) 
* [miniaudio](https://github.com/mackron/miniaudio)
* [RtMidi](https://github.com/thestk/rtmidi)
* [spdlog](https://github.com/gabime/spdlog)