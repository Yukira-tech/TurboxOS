# ============================================================
# @file pack_image.py
# @brief 内核镜像打包脚本
#
# 层级：
#   TurboBoxOS/tools/
#
# 项目内绝对路径：
#   TurboBoxOS/tools/pack_image.py
#
# 模块作用：
#   把内核二进制和初始文件打包成 turbox.img 自定义镜像。
#   镜像格式用魔数加文件表描述，boot 扇区可以直接解析，
#   bridge.py 也能反向读取。
#
# 使用者：
#   开发者在 build.py 生成 turbox.bin 后调用本脚本。
#
# 项目角色：
#   tools 层构建流程的最后一环，产出可启动镜像。
#
# 引入说明：
#   使用 Python 标准库 struct、sys、pathlib。
#
# 维护记录：
#   2026-08-30 初始创建
# ============================================================

import struct
import sys
from pathlib import Path

# 镜像魔数，boot 和 bridge 都靠它识别合法镜像
MAGIC = b"TBXIMG"

# 文件名长度必须和 fs/turboxfs.h 的 TFS_NAME_LEN 一致，
# 否则镜像和内核文件系统会错位
NAME_LEN = 32

# 项目根目录，本文件在 tools/ 下
ROOT = Path(__file__).resolve().parent.parent


def pack_image(kernel_bin, init_files, output):
    """
    把内核和初始文件打包成 turbox.img。

    内核固定以 "kernel" 名称作为第一个文件写入，
    这样 boot 扇区可以像读普通文件一样定位内核，不需要特殊逻辑。
    """
    entries = []  # 每项：(镜像内名称, 数据字节)

    kernel_data = Path(kernel_bin).read_bytes()
    entries.append(("kernel", kernel_data))

    # 初始文件名超长直接报错，和内核 TFS_NAME_LEN 契约保持一致
    for path in init_files:
        p = Path(path)
        name = p.name
        if len(name.encode("utf-8")) >= NAME_LEN:
            raise ValueError(f"文件名过长（>= {NAME_LEN} 字节）: {name}")
        entries.append((name, p.read_bytes()))

    # 镜像头 = 6 字节魔数 + 4 字节文件数
    header_size = len(MAGIC) + 4
    # 每个表项 = 32 字节名称 + 4 字节偏移 + 4 字节长度
    table_entry_size = NAME_LEN + 4 + 4
    data_offset = header_size + table_entry_size * len(entries)

    image = bytearray()
    image += MAGIC
    image += struct.pack("<I", len(entries))  # 小端 uint32

    # 先写文件表，再写数据区。写表时同步累加每个文件的数据偏移
    cursor = data_offset
    for name, data in entries:
        name_bytes = name.encode("utf-8")
        # 不足 32 字节补 NUL，保证表项定长，解析简单
        image += name_bytes.ljust(NAME_LEN, b"\x00")
        image += struct.pack("<I", cursor)   # 文件数据在镜像中的偏移
        image += struct.pack("<I", len(data)) # 文件数据长度
        cursor += len(data)

    # 文件表后面依次放每个文件的数据
    for _, data in entries:
        image += data

    Path(output).write_bytes(bytes(image))
    print(f"[pack_image.py] 镜像已生成: {output}（{len(entries)} 个文件，共 {len(image)} 字节）")


def main(argv):
    """
    命令行入口。
    用法：pack_image.py [kernel.bin] [init文件...] [-o 输出镜像]
    """
    args = list(argv)
    output = str(ROOT / "turbox.img")

    if "-o" in args:
        idx = args.index("-o")
        output = args[idx + 1]
        del args[idx:idx + 2]

    # 默认内核二进制在项目根目录
    kernel_bin = args[0] if args else str(ROOT / "turbox.bin")
    init_files = args[1:] if args else []

    if not Path(kernel_bin).exists():
        # 内核二进制缺失，多半是还没运行 build.py
        print(f"[pack_image.py] 错误: 找不到内核二进制 {kernel_bin}")
        print("[pack_image.py] 请先运行 tools/build.py 生成 turbox.bin")
        return 1

    pack_image(kernel_bin, init_files, output)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))