# ============================================================
# @file bridge.py
# @brief 链接层桥接器
#
# 层级：
#   TurboBoxOS/tools/
#
# 项目内绝对路径：
#   TurboBoxOS/tools/bridge.py
#
# 模块作用：
#   解析 turbox.img 或 fs_dump.json，把文件系统状态以 JSON
#   形式暴露给 web/terminal.js 消费。内核跑在裸机上无法直接
#   给浏览器喂数据，所以由本脚本充当中间层。
#
# 使用者：
#   前端 web/terminal.js 通过 fetch("fs_state.json") 使用本模块输出。
#
# 项目角色：
#   tools 层的数据桥，连接裸机内核产物和浏览器演示页面。
#
# 引入说明：
#   仅使用 Python 标准库 json/argparse/struct/pathlib，
#   不依赖第三方包。
#
# 维护记录：
#   2026-08-30 初始创建
# ============================================================

import argparse
import json
import struct
import sys
from pathlib import Path

# 镜像魔数和表项布局必须和 tools/pack_image.py 完全一致
MAGIC = b"TBXIMG"
NAME_LEN = 32

# 项目根目录，本文件在 tools/ 下
ROOT = Path(__file__).resolve().parent.parent

# 演示状态默认输出位置，web 终端 fetch 的就是这个文件
DEFAULT_STATE_PATH = ROOT / "web" / "fs_state.json"


def parse_image(image_path):
    """
    解析自定义 TBXIMG 镜像，返回文件列表。
    魔数不符或结构损坏时抛 ValueError，由调用方决定回退。
    """
    data = Path(image_path).read_bytes()
    if data[:6] != MAGIC:
        raise ValueError("不是合法的 TBXIMG 镜像（魔数不匹配）")

    # 文件头：6 字节魔数 + 4 字节文件数
    (count,) = struct.unpack_from("<I", data, 6)
    pos = 6 + 4

    entries = []
    for _ in range(count):
        # 每个表项：名称 32B + 偏移 4B + 长度 4B
        name_raw = data[pos:pos + NAME_LEN]
        name = name_raw.split(b"\x00", 1)[0].decode("utf-8")
        offset, length = struct.unpack_from("<II", data, pos + NAME_LEN)
        pos += NAME_LEN + 8

        # 二进制内容用替换符兜底，避免整体解码失败
        content = data[offset:offset + length].decode("utf-8", errors="replace")
        entries.append({"name": name, "content": content})

    return entries


def parse_fs_dump(dump_path):
    """读取内核导出的 fs_dump.json，返回文件列表。"""
    obj = json.loads(Path(dump_path).read_text(encoding="utf-8"))
    return obj.get("files", [])


def demo_files():
    """
    构造演示文件系统状态，模拟 user/demo.cpp 跑完后的结果。
    内容刻意贴近演示流程：创建脚本、写入、运行。
    """
    return [
        {
            "name": "hello.tbx",
            "content": "# Turbox 演示脚本\nprint hello from turbox\nprint microkernel online\n",
        },
        {
            "name": "notes.txt",
            "content": "Turbox 微内核：create / write / run 已可用。\n",
        },
        {
            "name": "boot.log",
            "content": "[kernel] hal_init ok\n[kernel] tfs_init ok\n[kernel] shell started\n",
        },
    ]


def write_state(files, output_path):
    """把文件列表写成 web 终端可 fetch 的 fs_state.json。"""
    payload = {"files": files}
    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    Path(output_path).write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(f"[bridge.py] 已写出 {output_path}（{len(files)} 个文件）")


def main(argv):
    """
    命令行入口。三种用法：
      --demo                  生成演示状态到 web/fs_state.json
      --image turbox.img      解析镜像并输出 JSON 到 stdout
      --dump fs_dump.json     读取内核导出并输出 JSON 到 stdout
    stdout 输出是 SPEC 4.7 的 JSON 行协议，供其他工具管道消费。
    """
    parser = argparse.ArgumentParser(description="Turbox 链接层桥接器")
    parser.add_argument("--demo", action="store_true", help="生成演示文件系统状态到 web/fs_state.json")
    parser.add_argument("--image", help="解析 turbox.img 镜像")
    parser.add_argument("--dump", help="读取内核导出的 fs_dump.json")
    parser.add_argument("--out", default=str(DEFAULT_STATE_PATH), help="--demo 模式的输出路径")
    args = parser.parse_args(argv)

    if args.demo:
        # 演示模式不需要真实镜像，直接生成一组演示文件
        write_state(demo_files(), args.out)
        return 0

    files = None
    if args.image:
        if not Path(args.image).exists():
            print(f"[bridge.py] 错误: 镜像不存在 {args.image}")
            return 1
        files = parse_image(args.image)
    elif args.dump:
        if not Path(args.dump).exists():
            print(f"[bridge.py] 错误: dump 文件不存在 {args.dump}")
            return 1
        files = parse_fs_dump(args.dump)
    else:
        # 未指定输入时默认尝试项目根目录的 turbox.img
        default_img = ROOT / "turbox.img"
        if default_img.exists():
            files = parse_image(default_img)
        else:
            print("[bridge.py] 未找到 turbox.img；用法：--demo / --image <img> / --dump <json>")
            return 1

    # 输出整体一行 JSON，便于管道逐行读取
    print(json.dumps({"files": files}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))