#!/usr/bin/env python3
import subprocess
import sys
import os
import re

XZ_MAGIC = b'\xfd7zXZ\x00'

def find_xz_stream_starts(data):
    starts = []
    pos = 0
    while True:
        idx = data.find(XZ_MAGIC, pos)
        if idx == -1:
            break
        starts.append(idx)
        pos = idx + 1
    return starts

def decompress_multistream_xz(input_path, output_path):
    with open(input_path, 'rb') as f:
        data = f.read()
    
    stream_starts = find_xz_stream_starts(data)
    print(f"Found {len(stream_starts)} XZ streams", file=sys.stderr)
    
    output = bytearray()
    total_streams = len(stream_starts)
    
    for i, start in enumerate(stream_starts):
        end = stream_starts[i + 1] if i + 1 < len(stream_starts) else len(data)
        stream_data = data[start:end]
        
        cmd = ['xz', '--decompress', '--format=xz', '--single-stream', '--stdout']
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        decomp_data, stderr = proc.communicate(input=stream_data)
        
        if proc.returncode == 0:
            output.extend(decomp_data)
            if (i + 1) % 100 == 0 or i == total_streams - 1:
                print(f"Stream {i+1}/{total_streams}: +{len(decomp_data)/1024/1024:.1f} MB, total: {len(output)/1024/1024:.1f} MB", file=sys.stderr)
        else:
            print(f"Stream {i+1} error: {stderr.decode()[:100]}", file=sys.stderr)
    
    with open(output_path, 'wb') as f:
        f.write(output)
    
    return len(output)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python3 decompress_dmg.py <input.xz> <output.raw>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    print(f"Decompressing {input_file}...", file=sys.stderr)
    size = decompress_multistream_xz(input_file, output_file)
    print(f"\nDone! {size/1024/1024:.1f} MB -> {output_file}")
