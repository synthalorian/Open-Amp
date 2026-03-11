#!/usr/bin/env python3
"""
Generate a simple test IR WAV file - sine wave impulse for testing the IR Loader.
"""
import struct
import math

# Parameters
sample_rate = 48000
duration_sec = 0.1  # 100ms impulse
frequency = 1000    # 1kHz sine wave
amplitude = 0.5

num_samples = int(sample_rate * duration_sec)

# Generate sine wave impulse
data = []
for i in range(num_samples):
    # Apply exponential decay envelope
    envelope = math.exp(-i / (sample_rate * 0.02))  # 20ms decay
    sample = amplitude * envelope * math.sin(2 * math.pi * frequency * i / sample_rate)
    # Convert to 16-bit signed integer
    sample_int = int(sample * 32767)
    sample_int = max(-32768, min(32767, sample_int))
    data.append(sample_int)

# WAV header
with open('/home/synth/projects/openamp/test_ir/test_sine_ir.wav', 'wb') as f:
    # RIFF chunk
    f.write(b'RIFF')
    data_size = num_samples * 2  # 16-bit mono
    file_size = 36 + data_size
    f.write(struct.pack('<I', file_size))
    f.write(b'WAVE')
    
    # fmt chunk
    f.write(b'fmt ')
    f.write(struct.pack('<I', 16))  # Subchunk size
    f.write(struct.pack('<H', 1))   # Audio format (PCM)
    f.write(struct.pack('<H', 1))   # Num channels (mono)
    f.write(struct.pack('<I', sample_rate))
    f.write(struct.pack('<I', sample_rate * 2))  # Byte rate
    f.write(struct.pack('<H', 2))   # Block align
    f.write(struct.pack('<H', 16))  # Bits per sample
    
    # data chunk
    f.write(b'data')
    f.write(struct.pack('<I', data_size))
    for sample in data:
        f.write(struct.pack('<h', sample))

print(f"Generated test IR: {num_samples} samples @ {sample_rate}Hz")
print(f"Duration: {duration_sec*1000:.1f}ms, Frequency: {frequency}Hz")
print(f"File: /home/synth/projects/openamp/test_ir/test_sine_ir.wav")
