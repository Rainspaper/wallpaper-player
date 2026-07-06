# Wallpaper Player

为了方便快速看下载的视频，你懂的。

一个轻量的 Wallpaper Engine 壁纸浏览器，直接扫描 Steam 工坊目录，展示所有壁纸的预览和描述，双击即可播放。

## 功能

- **浏览** — 网格展示所有壁纸，包含预览图、标题、标签、分级
- **筛选** — 按标题搜索、按分级筛选、按标签过滤
- **GIF 预览** — Wallpaper Engine 的 GIF 预览图在网格中直接播放
- **播放** — 双击任意壁纸调用系统播放器打开原始视频
- **多目录** — 支持添加多个壁纸目录，自动检测 Steam 安装路径
- **持久化** — 目录设置自动保存，下次打开无需重新配置

## 快速开始

首次启动会自动检测 Steam / Wallpaper Engine 目录，没找到的话手动选择 `steamapps/workshop/content/431960` 即可。

### 从源码构建

```
qmake WallpaperPlayer.pro
make -j$(nproc)
```

需要 Qt 6 + MinGW / MSVC。

## 打包分发

```
windeployqt release/WallpaperPlayer.exe --release
```

项目根目录下执行打包脚本即可生成 `WallpaperPlayer-Package.zip`，解压即用，无需安装 Qt。
