# TerrainGenerator
- UnrealEngineを使用した手続き型地形生成器です．
- 手続き型地形生成器はノイズ等のアルゴリズムを用いてランダム性のある地形をコンピューター上で自動的に生成するツールです．

# 起動方法
- 実行ファイル（`Windows/TerrainGenerator.exe`） から起動します．

# 操作方法
- タイトル画面の「地形生成開始」をクリックして地形生成を開始します．
- マウスを動かして視点の向きを動かします．
- キーボード操作
  - ＷＳＡＤキー：前後左右に移動する
  - スペースキー：ジャンプする
  - Ｐキー：ポーズ画面を開く
  - Ｍキー：ミニマップ表示・非表示を切り替える（デバッグ用の為，表示中は動作が重くなります）
  - ＋－キー：ミニマップの拡大・縮小する（マウスホイールでも操作可能）

# 使用プラグイン
- [RealtimeMeshComponent](https://github.com/TriAxis-Games/RealtimeMeshComponent)
- [UnrealFastNoise2](https://github.com/DoubleDeez/UnrealFastNoise2)
