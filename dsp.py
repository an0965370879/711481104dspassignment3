import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
from scipy.io import wavfile

# ==========================================
# 1. 畫 FIR 濾波器的頻率響應 (Magnitude Response)
# ==========================================
def plot_filter_response(filename='filter_coeffs.txt'):
    try:
        h = np.loadtxt(filename)
    except FileNotFoundError:
        print(f"找不到 {filename}")
        return

    w, h_freq = signal.freqz(h)
    freq_axis = w / np.pi 
    mag_db = 20 * np.log10(np.abs(h_freq) + 1e-10)

    # === 修改這裡：建立一個有兩張子圖 (Subplots) 的視窗 ===
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 10)) # 2行1列
    
    # --- 第一張圖：全頻域 (Full View) ---
    ax1.plot(freq_axis, mag_db, 'b')
    ax1.set_title('1. FIR Low-pass Filter Response (Full View)')
    ax1.set_ylabel('Magnitude (dB)')
    ax1.set_xlabel(r'Normalized Frequency ($\times \pi$ rad/sample)')
    ax1.grid(True)
    # 畫出紅線標示 Cutoff 位置
    ax1.axvline(1/441, color='r', linestyle='--', alpha=0.5, label='Cutoff (1/441)')
    ax1.legend()

    # --- 第二張圖：低頻放大 (Zoomed View) ---
    ax2.plot(freq_axis, mag_db, 'b')
    ax2.set_title('2. Low Frequency Detail (Zoomed In 0 ~ 0.2)')
    ax2.set_ylabel('Magnitude (dB)')
    ax2.set_xlabel(r'Normalized Frequency ($\times \pi$ rad/sample)')
    ax2.grid(True)
    ax2.axvline(1/441, color='r', linestyle='--', alpha=0.5, label='Cutoff')
    
    # 設定 X 軸範圍 (放大)
    ax2.set_xlim(0, 0.2) 
    
    # 自動調整排版，避免字疊在一起
    plt.tight_layout()
    
    # 存檔與顯示
    plt.savefig('filter_response_combined.png')
    plt.show()
    print("已儲存合併圖表：filter_response_combined.png")


# ==========================================
# 2. 畫輸入與輸出的頻譜圖 (Spectrogram)
# ==========================================
def plot_spectrograms(input_file='input.wav', output_file='output.wav'):
    # 讀取 wav 檔
    try:
        sr_in, data_in = wavfile.read(input_file)
        sr_out, data_out = wavfile.read(output_file)
    except FileNotFoundError:
        print("找不到 wav 檔案，請確認檔案名稱與路徑。")
        return

    # 如果是立體聲，只取左聲道來畫圖比較清楚
    if len(data_in.shape) > 1: data_in = data_in[:, 0]
    if len(data_out.shape) > 1: data_out = data_out[:, 0]

    plt.figure(figsize=(12, 8))

    # Input Spectrogram
    plt.subplot(2, 1, 1)
    plt.specgram(data_in, Fs=sr_in, NFFT=1024, noverlap=512, cmap='inferno')
    plt.title(f'Input Spectrogram ({sr_in} Hz)')
    plt.ylabel('Frequency (Hz)')
    plt.colorbar(format='%+2.0f dB')

    # Output Spectrogram
    plt.subplot(2, 1, 2)
    plt.specgram(data_out, Fs=sr_out, NFFT=256, noverlap=128, cmap='inferno')
    plt.title(f'Output Spectrogram ({sr_out} Hz)')
    plt.ylabel('Frequency (Hz)')
    plt.xlabel('Time (sec)')
    plt.colorbar(format='%+2.0f dB')

    plt.tight_layout()
    plt.savefig('spectrogram_comparison.png')
    plt.show()
    print("頻譜比較圖已儲存為 spectrogram_comparison.png")

if __name__ == "__main__":
    # 執行畫圖
    plot_filter_response()
    plot_spectrograms()