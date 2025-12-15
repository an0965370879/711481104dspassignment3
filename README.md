# Audio Resampling & Filter Analysis Project

這是一個音訊訊號處理專案，實作了基於 FIR 濾波器的取樣率轉換器 (Sample Rate Converter)。

**主要功能：**
1. **C 語言核心 (`main.c`)**：讀取 WAV 檔案，透過多速率訊號處理 (Multirate DSP) 將音訊從 44.1kHz 降頻至 8kHz，並輸出轉換後的音訊與濾波器係數。
2. **Python 分析工具 (`dsp.py`)**：讀取 C 程式生成的係數檔 (`.txt`)，繪製濾波器的時域波形與頻率響應圖，以驗證濾波器設計特性。

---

##  使用步驟 (Usage)

### 步驟 1：編譯 C 程式
開啟終端機 (Terminal/CMD)，切換到檔案所在目錄，執行以下指令：

```bash
gcc main.c -o audio -lm
```

### 步驟 2：執行音訊轉換
執行編譯好的程式：

* **Windows:** `.\audio.exe`

* **Mac/Linux:** `./audio`

**操作流程：**
1. 程式會列出當前資料夾下所有的 `.wav `檔案。
2. 輸入編號選擇要處理的檔案。
3. 程式將自動執行：
    * 產生轉換後的音訊檔 (檔名格式：`原始檔名_out.wav`)。
    * 產生濾波器係數檔 (檔名格式：`原始檔名_coeffs.txt`)。

### 步驟 3：繪製濾波器圖形

使用 Python 腳本讀取步驟 2 產生的 `.txt `係數檔並畫圖：
```bash
python dsp.py
```
**操作流程：**
1. 腳本會列出目錄下所有的 `.txt` 檔案。
2. 選擇剛才由 C 程式生成的係數檔 (例如 `music_coeffs.txt`)。
3. 視窗將顯示濾波器的 **時域響應** 與 **頻率響應**。
---

## 程式碼詳解 (Code Explanation: main.c)
本專案的核心邏輯位於 main.c，以下針對四個主要部分進行解析：
### 1. 結構定義與函式庫
引入了標準 I/O 與數學庫，並使用了 <dirent.h> 來實現讀取資料夾的功能。
```C
// 定義 WAV 標頭結構 (標準 44 bytes)
typedef struct {
    char riffID[4];     // "RIFF" 標記
    uint32_t chunkSize; // 檔案大小
    // ... (其他欄位)
    uint32_t sampleRate;// 取樣率 (如 44100)
    uint32_t dataSize;  // 音訊資料總長度
} WavHeader;
```
* **WavHeader:** 透過這個結構體 (Structure)，程式可以直接讀取 WAV 檔案開頭的 44 bytes，快速獲取取樣率、聲道數等關鍵資訊。
### 2. 輔助功能 (檔案 I/O)
* **select_file():**
    * 使用 opendir 與 readdir 遍歷當前目錄。
    * 過濾出 .wav 檔並排除程式自己生成的 _out.wav 檔。
    * 讓使用者透過數字選單選擇檔案。
*  **write_wav():**
    * 負責將處理完的 PCM 數據包裝回標準 WAV 格式。
    * 自動填寫 Header 資訊 (如 RIFF, WAVE, fmt 區塊)，確保播放器能正確辨識。
### 3. 濾波器設計 (Filter Design)
位於 design_filter 函式中，負責計算 FIR 低通濾波器的係數 h[n]。
* **目的：** 在降頻 (Downsampling) 前濾除高頻訊號，防止混疊 (Aliasing)。

* **數學原理：** 

    **1. Sinc 函數：** 計算理想低通濾波器。

    $$h_d[n] = \frac{\sin(\omega_c (n - center))}{\pi (n - center)}$$

    其中截止頻率  $\omega_c = \pi / M$ (取決於降頻倍率)。

    **2. Hamming Window :** 修飾 Sinc 函數以減少吉布斯現象 (Gibbs Phenomenon)，使濾波器更平滑。

    $$w[n] = 0.54 - 0.46 \cos\left(\frac{2\pi n}{P-1}\right)$$


    **3.增益補償 (Gain)：** 係數最後乘以 $L$ (升頻倍率)，以補償補零造成的能量損失。

---
### 4. 核心轉換邏輯 (Main Loop)這是程式最複雜的部分，實作了 Resampling by Convolution。
```C
// 參數設定
const int L = 80;   // 升頻因子 (Upsampling)
const int M = 441;  // 降頻因子 (Downsampling)
// 目標取樣率 = 44100 * (80/441) = 8000 Hz
```
**程式使用單一迴圈同時完成「升頻 -> 濾波 -> 降頻」：**
```C
for (int m = 0; m < num_output_samples; m++) {
    double sum = 0.0;
    for (int k = 0; k < P; k++) {
        // 計算對應的升頻索引
        long long upsampled_idx = (long long)m * M - k;
        
        // 【虛擬升頻技術】
        // 只有當索引是 L 的倍數時，才代表對應到原始訊號的非零值
        if (upsampled_idx % L == 0) {
            long long input_idx = upsampled_idx / L;
            if (input_idx >= 0 && input_idx < num_input_samples) {
                // 執行卷積 (Convolution)
                sum += input_data[input_idx * 2 + ch] * h[k];
            }
        }
    }
    output_data[m] = (short)sum; // 存入結果
}
```
* **虛擬升頻 (Virtual Upsampling)：** 程式並沒有真的在記憶體中插入 0 (補零)，而是透過 `if (upsampled_idx % L == 0) `判斷。如果不是 L 的倍數，代表該位置是補零點，乘積必為 0，因此直接跳過運算。這大大提升了運算效率。