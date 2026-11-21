# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""Make S_Water_FillCan.wav, the splash of water poured into the watering can at the well (Docs/Systems/14-Audio.md).

    python Art/Audio/make_water_fill_can.py              writes Art/Export/S_Water_FillCan.wav
    python Art/Audio/make_water_fill_can.py --check-only checks the sound and writes nothing

Plain Python 3, standard library only. The random generator has a fixed seed, so every run gives the same file; a change in
the WAV means the script changed. The script is the source: change it and run it again, don't edit the WAV.

The sound, about 0.7 s: a slosh of band-passed noise, a run of small bubbles (damped sine waves that rise in pitch as they
die away, the way an air bubble in water rings), a faint ring of the tin can, then a fade. 48 kHz, 16-bit, mono.
"""

import argparse
import math
import os
import random
import struct
import sys
import wave

SEED = 20261121
SAMPLE_RATE = 48000
DURATION_SECONDS = 0.7
OUTPUT_NAME = "S_Water_FillCan.wav"

PEAK_DBFS = -3.0
PEAK_TOLERANCE_DB = 0.1
MIN_SECONDS = 0.4
MAX_SECONDS = 1.2
MAX_DC_OFFSET = 0.001
MAX_EDGE_SAMPLE = 0.001

SLOSH_BAND_HZ = (500.0, 3000.0)
SLOSH_ATTACK_SECONDS = 0.010
SLOSH_DECAY_SECONDS = 0.120
SLOSH_GAIN = 0.5
BUBBLE_COUNT = (12, 20)
BUBBLE_START_SECONDS = (0.02, 0.45)
BUBBLE_HZ = (400.0, 1800.0)
BUBBLE_RISE = (0.10, 0.30)
BUBBLE_RISE_SECONDS = (0.030, 0.080)
BUBBLE_DECAY_SECONDS = (0.015, 0.040)
BUBBLE_GAIN = (0.10, 0.30)
RING_PARTIALS_HZ = (850.0, 2150.0, 3900.0)
RING_DECAY_SECONDS = 0.250
RING_GAIN = 0.125  # about -18 dB
HIGH_PASS_HZ = 120.0
LOW_PASS_HZ = 9000.0
FADE_OUT_SECONDS = 0.060
FADE_IN_SECONDS = 0.002


def biquad(samples, kind, frequency, q=0.7071):
    """An RBJ biquad filter, "low" or "high" pass."""
    omega = 2.0 * math.pi * frequency / SAMPLE_RATE
    alpha = math.sin(omega) / (2.0 * q)
    cosine = math.cos(omega)
    if kind == "low":
        b0 = b2 = (1.0 - cosine) / 2.0
        b1 = 1.0 - cosine
    else:
        b0 = b2 = (1.0 + cosine) / 2.0
        b1 = -(1.0 + cosine)
    a0 = 1.0 + alpha
    a1 = -2.0 * cosine
    a2 = 1.0 - alpha
    b0, b1, b2, a1, a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0
    x1 = x2 = y1 = y2 = 0.0
    out = []
    for x0 in samples:
        y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        out.append(y0)
        x2, x1 = x1, x0
        y2, y1 = y1, y0
    return out


def slosh(count, rng):
    noise = [rng.uniform(-1.0, 1.0) for _ in range(count)]
    noise = biquad(biquad(noise, "high", SLOSH_BAND_HZ[0]), "low", SLOSH_BAND_HZ[1])
    out = []
    for index, value in enumerate(noise):
        time = index / SAMPLE_RATE
        attack = min(1.0, time / SLOSH_ATTACK_SECONDS)
        decay = math.exp(-time / SLOSH_DECAY_SECONDS)
        out.append(value * attack * decay * SLOSH_GAIN)
    return out


def add_bubble(buffer, rng):
    start = int(rng.uniform(*BUBBLE_START_SECONDS) * SAMPLE_RATE)
    base_hz = rng.uniform(*BUBBLE_HZ)
    rise = rng.uniform(*BUBBLE_RISE)
    rise_seconds = rng.uniform(*BUBBLE_RISE_SECONDS)
    decay_seconds = rng.uniform(*BUBBLE_DECAY_SECONDS)
    gain = rng.uniform(*BUBBLE_GAIN)
    length = int(decay_seconds * 6.0 * SAMPLE_RATE)
    phase = 0.0
    for index in range(length):
        if start + index >= len(buffer):
            break
        time = index / SAMPLE_RATE
        frequency = base_hz * (1.0 + rise * min(1.0, time / rise_seconds))
        phase += 2.0 * math.pi * frequency / SAMPLE_RATE
        attack = min(1.0, time / 0.002)
        buffer[start + index] += math.sin(phase) * math.exp(-time / decay_seconds) * attack * gain


def add_ring(buffer, rng):
    for frequency in RING_PARTIALS_HZ:
        phase = rng.uniform(0.0, 2.0 * math.pi)
        for index in range(len(buffer)):
            time = index / SAMPLE_RATE
            envelope = math.exp(-time / RING_DECAY_SECONDS) * min(1.0, time / 0.005)
            buffer[index] += math.sin(2.0 * math.pi * frequency * time + phase) * envelope * RING_GAIN / len(RING_PARTIALS_HZ)


def synthesize():
    rng = random.Random(SEED)
    count = int(DURATION_SECONDS * SAMPLE_RATE)
    buffer = slosh(count, rng)
    for _ in range(rng.randint(*BUBBLE_COUNT)):
        add_bubble(buffer, rng)
    add_ring(buffer, rng)
    buffer = biquad(buffer, "high", HIGH_PASS_HZ)
    buffer = biquad(buffer, "low", LOW_PASS_HZ)

    fade_out = int(FADE_OUT_SECONDS * SAMPLE_RATE)
    fade_in = int(FADE_IN_SECONDS * SAMPLE_RATE)
    for index in range(fade_in):
        buffer[index] *= index / fade_in
    for index in range(fade_out):
        buffer[count - 1 - index] *= index / fade_out
    buffer[-1] = 0.0

    peak = max(abs(value) for value in buffer)
    scale = 10.0 ** (PEAK_DBFS / 20.0) / peak
    return [value * scale for value in buffer]


def check(samples):
    """The problems with the sound, as a list of messages; empty when it is fine."""
    problems = []
    seconds = len(samples) / SAMPLE_RATE
    if not MIN_SECONDS <= seconds <= MAX_SECONDS:
        problems.append("length {0:.3f} s is outside {1} to {2} s".format(seconds, MIN_SECONDS, MAX_SECONDS))
    peak = max(abs(value) for value in samples)
    peak_db = 20.0 * math.log10(peak) if peak > 0.0 else -math.inf
    if peak_db > PEAK_DBFS + PEAK_TOLERANCE_DB:
        problems.append("peak {0:.2f} dBFS is above {1} dBFS".format(peak_db, PEAK_DBFS))
    if peak_db == -math.inf:
        problems.append("the sound is silent")
    dc = sum(samples) / len(samples)
    if abs(dc) >= MAX_DC_OFFSET:
        problems.append("DC offset {0:.5f} is too large".format(dc))
    if abs(samples[0]) > MAX_EDGE_SAMPLE or abs(samples[-1]) > MAX_EDGE_SAMPLE:
        problems.append("the sound doesn't start and end at silence (it would click)")
    return problems


def write_wav(samples, path):
    frames = b"".join(struct.pack("<h", max(-32768, min(32767, int(round(value * 32767.0))))) for value in samples)
    with wave.open(path, "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(frames)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check-only", action="store_true", help="check the sound and write nothing")
    arguments = parser.parse_args()

    samples = synthesize()
    problems = check(samples)
    if problems:
        for problem in problems:
            print("make_water_fill_can: " + problem, file=sys.stderr)
        return 1
    print("make_water_fill_can: {0:.3f} s, peak {1:.2f} dBFS, checks passed".format(
        len(samples) / SAMPLE_RATE, 20.0 * math.log10(max(abs(value) for value in samples))))
    if arguments.check_only:
        return 0

    export_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Export")
    path = os.path.normpath(os.path.join(export_folder, OUTPUT_NAME))
    write_wav(samples, path)
    print("make_water_fill_can: wrote " + path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
