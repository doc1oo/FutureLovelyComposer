# FutureLovelyComposer
 Prototype of the next generation Lovely Composer

It's a personal project for now. (work in progress)

![image](https://github.com/user-attachments/assets/d1613e6e-4de5-4771-b939-87570e299c42)

## Dev tools
* Windows 11 
* Godot 4.3
* Visual Studio Community 2022 (『C++によるデスクトップ開発』をインストール) 17.11
* VSCode
* CMake

## コンパイルと実行
* コマンドプロンプトでcd FutureLovelyComposerで移動したあと、./build.batを実行する
* 詳細なコンパイル方法の指定はCMake(CMakelist.txt)で

## 説明
今のところGodot側のプログラム（GUI中心）と、GDTuneと呼ぶ自作オーディオシステムGDExtensionに大きく分かれています。
* GDTuneのソースコードはgdtune/とlibs/以下
* Godot側のソースコードはgodot_project/以下

## License
一部のソースコードはclap-info、clap-hostを参考に書いており、miniaudioに関する一部のコードは公式サイトのチュートリアルから引用しています。

使用しているライブラリ等についてはそれぞれに定められたライセンスになります。（licenses/を参照のこと）
* [clap](https://github.com/free-audio/clap)
* [clap-info](https://github.com/free-audio/clap-info)
* [clap-helper](https://github.com/free-audio/clap-helpers)
* [clap-host](https://github.com/free-audio/clap-host)
* [godot](https://github.com/godotengine/godot)
* [godot-cpp](https://github.com/godotengine/godot-cpp) 
* [jsoncpp](https://github.com/open-source-parsers/jsoncpp) 
* [miniaudio](https://github.com/mackron/miniaudio)
* [RtMidi](https://github.com/thestk/rtmidi)
