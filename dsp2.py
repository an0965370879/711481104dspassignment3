import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import freqz

file_path = "filter_coeffs.txt"

FS_IN = 44100  # 輸入取樣率
FS_OUT = 8000  # 輸出取樣率

def plot_coefficients_from_txt(file_path):
    try:
        coefficients = np.loadtxt(file_path)
        
    except FileNotFoundError:
        print(f"錯誤：找不到檔案 '{file_path}'。請確認 C 程式碼已執行並生成了 output.txt。")
        return
    except Exception as e:
        print(f"讀取檔案時發生錯誤: {e}")
        return

    filter_length = len(coefficients)
    
    print(f"繪製時域響應 (長度: {filter_length})...")
    plt.figure(figsize=(12, 5))
    
    plt.plot(coefficients)
    
    center_index = (filter_length - 1) // 2
    plt.axvline(center_index, color='r', linestyle='--', label=f'Center Index ({center_index})')

    plt.title(f'FIR Filter Coefficients (Length = {filter_length}) - Time Domain')
    plt.xlabel('Coefficient Index (n)')
    plt.ylabel('Amplitude ($h[n]$)')
    plt.grid(True, linestyle='--')
    plt.legend()
    plt.tight_layout()
    w, H = freqz(coefficients, [1], worN=8192, fs=FS_IN)
    
    magnitude_db = 20 * np.log10(np.abs(H))
    
    print(f"繪製頻域響應...")
    plt.figure(figsize=(12, 6))
    
    plt.plot(w, magnitude_db)
    
    # 標記目標奈奎斯特頻率 (4000 Hz)
    target_nyquist = FS_OUT / 2
    plt.axvline(target_nyquist, color='r', linestyle='--', label=f'Target Nyquist ({target_nyquist} Hz)')
    
    plt.title('FIR Filter Magnitude Response - Frequency Domain')
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Magnitude (dB)')
    plt.grid(True, which='both', linestyle='--', alpha=0.7)
    plt.xlim(0, 6000) # 繪製到輸入奈奎斯特頻率 (22050 Hz)
    plt.ylim(-200, 100) # 調整 Y 軸範圍以觀察阻帶衰減
    plt.legend()
    plt.tight_layout()
    
    plt.show()

plot_coefficients_from_txt(file_path)