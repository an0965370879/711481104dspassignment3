#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

// 定義 WAV 標頭結構 (標準 44 bytes)
typedef struct {
    char riffID[4];         // "RIFF"
    uint32_t chunkSize;     // 檔案總大小 - 8
    char waveID[4];         // "WAVE"
    char fmtID[4];          // "fmt "
    uint32_t fmtChunkSize;  // 通常是 16
    uint16_t audioFormat;   // 1 代表 PCM
    uint16_t numChannels;   // 聲道數 (Stereo = 2)
    uint32_t sampleRate;    // 取樣率 (Input: 44100)
    uint32_t byteRate;      // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;    // numChannels * bitsPerSample/8
    uint16_t bitsPerSample; // 位元深度 (16 bits)
    char dataID[4];         // "data"
    uint32_t dataSize;      // 音訊資料總 bytes 數
} WavHeader;

// 輔助函式：寫入 WAV 檔案
void write_wav(const char *filename, short *buffer, int num_samples, int sample_rate) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) { printf("Error opening output file\n"); return; }

    WavHeader header;
    // 填寫 Header 固定資訊
    sprintf(header.riffID, "RIFF");
    sprintf(header.waveID, "WAVE");
    sprintf(header.fmtID, "fmt ");
    sprintf(header.dataID, "data");
    
    header.fmtChunkSize = 16;
    header.audioFormat = 1; // PCM
    header.numChannels = 2; // Stereo [cite: 10]
    header.sampleRate = sample_rate;
    header.bitsPerSample = 16;
    header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
    header.byteRate = header.sampleRate * header.blockAlign;
    header.dataSize = num_samples * header.blockAlign; // 資料區大小
    header.chunkSize = 36 + header.dataSize;

    fwrite(&header, sizeof(WavHeader), 1, fp); // 寫入 Header
    fwrite(buffer, sizeof(short), num_samples * 2, fp); // 寫入資料 (Stereo * 2)
    fclose(fp);
    printf("Output file written: %s (%d Hz)\n", filename, sample_rate);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 產生濾波器係數
// h: 輸出的係數陣列
// P: 濾波器長度 (1025)
// L: 升頻倍數 (80)
// M: 降頻倍數 (441)
void design_filter(double *h, int P, int L, int M) {
    double omega_c = M_PI / M; // Cutoff frequency: min(pi/L, pi/M) -> pi/441
    int center = (P - 1) / 2;  // 中心點: 512
    
    for (int n = 0; n < P; n++) {
        // 1. 計算理想 Low-pass (Sinc function) [cite: 27]
        double hd;
        if (n == center) {
            hd = omega_c / M_PI; // 處理分母為 0 的情況 (L'Hopital's rule)
        } else {
            double x = n - center;
            hd = sin(omega_c * x) / (M_PI * x);
        }

        // 2. 計算 Hamming Window 
        // Hamming: 0.54 - 0.46 * cos(2*pi*n / (P-1))
        double w = 0.54 - 0.46 * cos(2.0 * M_PI * n / (P - 1));

        // 3. 結合並套用 Gain 
        // h[n] = hd * w * Gain(L)
        h[n] = hd * w * L; 
    }
}

int main() {
    // 1. 參數設定
    const int L = 80;
    const int M = 441;
    const int P = 1025; // Filter length 
    
    // 2. 讀取 Input 檔案
    FILE *fin = fopen("input.wav", "rb");
    if (!fin) { printf("Cannot open input.wav\n"); return -1; }
    
    WavHeader header;
    fread(&header, sizeof(WavHeader), 1, fin);
    
    int num_input_samples = header.dataSize / header.blockAlign;
    // 分配記憶體讀取立體聲資料 (Input)
    short *input_data = (short *)malloc(header.dataSize);
    fread(input_data, 1, header.dataSize, fin);
    fclose(fin);

    printf("Input Read: %d samples per channel.\n", num_input_samples);

    // 3. 準備濾波器
    double *h = (double *)malloc(P * sizeof(double));
    design_filter(h, P, L, M);

    // 4. 建立一個檔案來存係數
    FILE *fp_coeff = fopen("filter_coeffs.txt", "w");
    if (fp_coeff) {
        for (int i = 0; i < P; i++) {
            // 把每個 h[n] 寫入檔案，換行分隔
            fprintf(fp_coeff, "%.15f\n", h[i]);
        }
        fclose(fp_coeff);
        printf("Filter coefficients saved to filter_coeffs.txt\n");
    }


    // 5. 計算輸出大小
    // Output samples approx = Input * L / M
    int num_output_samples = (int)((long long)num_input_samples * L / M);
    short *output_data = (short *)malloc(num_output_samples * 2 * sizeof(short)); // *2 for Stereo

    printf("Processing... (Target Output: %d samples)\n", num_output_samples);

    // 6. 核心處理迴圈 (Stereo: Channel 0=Left, 1=Right)
    for (int ch = 0; ch < 2; ch++) {
        for (int m = 0; m < num_output_samples; m++) {
            double sum = 0.0;
            
            // 卷積核心: y[m] = sum( h[k] * x_upsampled[m*M - k] )
            // 我們只遍歷 filter 係數 k
            for (int k = 0; k < P; k++) {
                // 虛擬 Up-sampling 索引
                long long upsampled_idx = (long long)m * M - k;
                
                // 檢查是否落在非零點上 (即 upsampled_idx 必須是 L 的倍數)
                if (upsampled_idx % L == 0) {
                    long long input_idx = upsampled_idx / L;
                    
                    // 邊界檢查 (Zero padding logic)
                    if (input_idx >= 0 && input_idx < num_input_samples) {
                        // 讀取 Input (注意立體聲: index * 2 + ch)
                        short sample = input_data[input_idx * 2 + ch];
                        sum += sample * h[k];
                    }
                }
            }
            
          

            // 存入 Output (轉回 short 並限制範圍)
            if (sum > 32767) sum = 32767;
            if (sum < -32768) sum = -32768;
            output_data[m * 2 + ch] = (short)sum;
        }
    }

    // 7. 寫入檔案
    write_wav("output.wav", output_data, num_output_samples, 8000); // [cite: 9]

    // 釋放記憶體
    free(input_data);
    free(output_data);
    free(h);
    
    printf("Done.\n");
    return 0;
}