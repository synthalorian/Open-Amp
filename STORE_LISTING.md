# Play Store Listing — Open Amp (price TBD after hardware test)

## App name
Open Amp — Guitar Amplifier

## Short description (80 chars max)
A real guitar amp in your pocket. Amp sim, effects, tuner, looper. USB in.

## Full description (4000 chars max)
Plug in. Turn up. No strings attached — except yours.

Open Amp is a full guitar signal chain running on your phone: noise gate,
compressor, EQ, amp simulator with gain/drive/3-band tone/presence/master,
cabinet simulation with impulse responses, distortion, modulation (chorus,
flanger, phaser, tremolo, vibrato), delay, reverb, acoustic simulator,
harmonizer — plus a chromatic tuner, a looper, and a metronome.

**Built for players**
🎸 Amp simulator with full tone stack and master section
🔊 Cabinet sim + custom IR loading (load your own WAV impulse responses)
🎛️ 12+ effects across the chain, each with real-time controls
🎯 Chromatic tuner with note and cents readout
🔁 Looper (up to 60s) and metronome for practice sessions
💾 Presets — save and recall your entire rig
🔌 USB audio interface support with automatic device selection and
low-latency Oboe audio (hot-swap interfaces mid-session)

**No junk**
No accounts. No ads. No tracking. No in-app purchases of "premium tone."
Your signal never leaves your phone — audio is processed on-device in real
time, always.

Made by a guitarist who got tired of amp sims that cost more than the amp.

## Category
Music & Audio

## Content rating answers (IARC)
- All content categories: none. No social/UGC, no location.
- Expected rating: Everyone

## Data safety (Play Console form)
- Microphone: used for real-time on-device processing, NOT collected or shared
- No data collected or shared overall
- Privacy policy URL:
  https://github.com/synthalorian/Open-Amp/blob/main/PRIVACY.md

## Store assets (in repo)
- store-assets/icon-512.png
- store-assets/feature-1024x500.png
- Screenshots: TODO — capture on-device after hardware test (main UI,
  tuner active, effects pages) 1080x2400

## Notes
- applicationId: com.synthalorian.openamp (changed pre-upload 2026-09-11, IMMUTABLE)
- Upload key: ~/.keystores/openamp-upload.jks
- AAB: android/app/build/outputs/bundle/release/app-release.aab
- GATE: do NOT upload until synth confirms guitar-in → processed-audio-out
  works on-device (Oboe rewire 2026-09-11 is UNVERIFIED pending USB-C adapter)
- Price: propose $4.99 (amp sims run $4.99-20; open-source credibility +
  one-time price is the wedge) — FINAL CALL AFTER HARDWARE TEST
