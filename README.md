# 🌫️ Smart Air Quality Pattern Recognition (IoT + Edge AI)

Sistem ini mengklasifikasikan kualitas udara menjadi **3 kondisi**:

- ✅ **Clean** — Udara normal & stabil  
- 🟡 **Stale** — Udara mulai pengap (kenaikan perlahan)  
- 🔴 **Hazardous** — Lonjakan gas mendadak (spike)

Pendekatan yang dibandingkan:
1. **If–Else Threshold (rule-based)**
2. **Edge AI (Machine Learning di device)**

Sensor:
- Sensor Gas (MQ series)
- Sensor Suhu & Kelembapan

---
## 🎯 Tujuan
- Mendeteksi pola kualitas udara secara real-time
- Membandingkan akurasi metode threshold dan machine learning
- Menganalisis pentingnya suhu dan kelembapan dalam meningkatkan akurasi klasifikasi

## ⚙️ Alur Sistem
Sensor → Microcontroller → Pengolahan Data → Klasifikasi → Output

## 🧪 Dataset
Dataset dikumpulkan dari eksperimen fisik dengan minimal **50 sampel untuk tiap kelas** menggunakan fitur:
- Nilai gas
- Suhu
- Kelembapan

Kelompok 2 CaAs