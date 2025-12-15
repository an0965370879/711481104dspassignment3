import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import freqz
import os  # [新增] 用於讀取目錄

FS_IN = 44100  # 輸入取樣率
FS_OUT = 8000  # 輸出取樣率

def select_txt_file():
    """列出當前目錄下的 .txt 檔案並讓使用者選擇"""
    # 搜尋當前目錄下所有的 .txt 檔案
    files = [f for f in os.listdir('.') if f.endswith('.txt') and os.path.isfile(f)]
    
    if not files:
        print("當前目錄下找不到任何 .txt 檔案。")
        return None

    print("=========================================")
    print("發現以下係數檔案：")
    for i, f in enumerate(files):
        print(f"[{i+1}] {f}")
    print("=========================================")
    
    while True:
        try:
            choice = input(f"請輸入檔案編號 (1-{len(files)}): ")
            idx = int(choice) - 1
            if 0 <= idx < len(files):
                return files[idx]
            else:
                print("輸入無效，請重新輸入。")
        except ValueError:
            print("請輸入數字。")

def plot_coefficients_from_txt(file_path):
    if not file_path: return

    try:
        print(f"正在讀取: {file_path} ...")
        coefficients = np.loadtxt(file_path)
    except Exception as e:
        print(f"讀取檔案時發生錯誤: {e}")
        return

    filter_length = len(coefficients)
    
    # --- 繪圖 1: 時域 ---
    print(f"繪製時域響應 (長度: {filter_length})...")
    plt.figure(figsize=(12, 5))
    plt.plot(coefficients)
    
    center_index = (filter_length - 1) // 2
    plt.axvline(center_index, color='r', linestyle='--', label=f'Center Index ({center_index})')

    plt.title(f'FIR Filter Coefficients - {file_path}\n(Length = {filter_length})')
    plt.xlabel('Coefficient Index (n)')
    plt.ylabel('Amplitude ($h[n]$)')
    plt.grid(True, linestyle='--')
    plt.legend()
    plt.tight_layout()

    # --- 繪圖 2: 頻域 ---
    # 注意: 這裡計算頻率響應時，fs 的設定取決於你如何定義這組係數的基頻
    # 若這是升頻後的濾波器，頻譜分析的解讀會比較複雜。
    # 這裡維持原本邏輯，以 FS_IN 為基準觀察形狀。
    w, H = freqz(coefficients, [1], worN=8192, fs=FS_IN)
    magnitude_db = 20 * np.log10(np.abs(H) + 1e-10) # 加一個極小值避免 log(0)
    
    print(f"繪製頻域響應...")
    plt.figure(figsize=(12, 6))
    plt.plot(w, magnitude_db)
    
    target_nyquist = FS_OUT / 2
    plt.axvline(target_nyquist, color='r', linestyle='--', label=f'Target Nyquist ({target_nyquist} Hz)')
    
    plt.title(f'Magnitude Response - {file_path}')
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Magnitude (dB)')
    plt.grid(True, which='both', linestyle='--', alpha=0.7)
    
    # 根據你的需求設定顯示範圍
    plt.xlim(0, 6000) 
    plt.ylim(-200, 50) 
    
    plt.legend()
    plt.tight_layout()
    plt.show()

# --- 主程式 ---
if __name__ == "__main__":
    selected_file = select_txt_file()
    if selected_file:
        plot_coefficients_from_txt(selected_file)