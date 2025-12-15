#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h> // 用於讀取目錄

// 定義 WAV 標頭結構
typedef struct {
    char riffID[4];
    uint32_t chunkSize;
    char waveID[4];
    char fmtID[4];
    uint32_t fmtChunkSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char dataID[4];
    uint32_t dataSize;
} WavHeader;

// 輔助函式：列出並選擇 WAV 檔案
int select_file(char *selected_filename) {
    DIR *d;
    struct dirent *dir;
    char file_list[100][256];
    int count = 0;

    d = opendir("."); // 開啟當前目錄
    if (d) {
        printf("=========================================\n");
        printf("File list in current directory:\n");
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, ".wav") != NULL) {
                // 排除 output 檔案
                if (strstr(dir->d_name, "_out.wav") == NULL) {
                    strcpy(file_list[count], dir->d_name);
                    printf("[%d] %s\n", count + 1, file_list[count]);
                    count++;
                    if (count >= 100) break;
                }
            }
        }
        closedir(d);
    } else {
        printf("Error: Could not open directory.\n");
        return 0;
    }

    if (count == 0) {
        printf("No .wav files found.\n");
        return 0;
    }

    printf("=========================================\n");
    printf("Please select a file number (1-%d): ", count);
    
    int choice;
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > count) {
        printf("Invalid selection.\n");
        return 0;
    }

    strcpy(selected_filename, file_list[choice - 1]);
    return 1;
}

// 輔助函式：寫入 WAV 檔案
void write_wav(const char *filename, short *buffer, int num_samples, int sample_rate) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) { printf("Error opening output file\n"); return; }

    WavHeader header;
    sprintf(header.riffID, "RIFF");
    sprintf(header.waveID, "WAVE");
    sprintf(header.fmtID, "fmt ");
    sprintf(header.dataID, "data");
    
    header.fmtChunkSize = 16;
    header.audioFormat = 1; 
    header.numChannels = 2; 
    header.sampleRate = sample_rate;
    header.bitsPerSample = 16;
    header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
    header.byteRate = header.sampleRate * header.blockAlign;
    header.dataSize = num_samples * header.blockAlign; 
    header.chunkSize = 36 + header.dataSize;

    fwrite(&header, sizeof(WavHeader), 1, fp); 
    fwrite(buffer, sizeof(short), num_samples * 2, fp); 
    fclose(fp);
    printf("Output file written: %s (%d Hz)\n", filename, sample_rate);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 產生濾波器係數
void design_filter(double *h, int P, int L, int M) {
    double omega_c = M_PI / M; 
    int center = (P - 1) / 2;  
    
    for (int n = 0; n < P; n++) {
        double hd;
        if (n == center) {
            hd = omega_c / M_PI; 
        } else {
            double x = n - center;
            hd = sin(omega_c * x) / (M_PI * x);
        }

        double w = 0.54 - 0.46 * cos(2.0 * M_PI * n / (P - 1));
        h[n] = hd * w * L; 
    }
}

int main() {
    // 1. 參數設定
    const int L = 80;
    const int M = 441;
    const int P = 1025; 

    // 2. 檔案選擇邏輯
    char input_filename[256];
    if (!select_file(input_filename)) {
        return -1; // 選擇失敗或取消
    }

    printf("Processing file: %s\n", input_filename);

    // 3. 讀取 Input 檔案
    FILE *fin = fopen(input_filename, "rb");
    if (!fin) { printf("Cannot open %s\n", input_filename); return -1; }
    
    WavHeader header;
    fread(&header, sizeof(WavHeader), 1, fin);
    
    int num_input_samples = header.dataSize / header.blockAlign;
    short *input_data = (short *)malloc(header.dataSize);
    fread(input_data, 1, header.dataSize, fin);
    fclose(fin);

    printf("Input Read: %d samples per channel.\n", num_input_samples);

    // 4. 準備濾波器
    double *h = (double *)malloc(P * sizeof(double));
    design_filter(h, P, L, M);

    // ==========================================
    // 5. 儲存濾波器係數到 txt 檔案
    // ==========================================
    char coeff_filename[300];
    strcpy(coeff_filename, input_filename); // 複製輸入檔名 (例如 "song.wav")
    
    // 去除 .wav 副檔名
    char *dot_ptr = strrchr(coeff_filename, '.');
    if (dot_ptr) *dot_ptr = '\0'; // 變成 "song"
    
    // 加上後綴
    strcat(coeff_filename, "_coeffs.txt"); // 變成 "song_coeffs.txt"

    FILE *fp_coeff = fopen(coeff_filename, "w"); // 開啟動態檔名
    if (fp_coeff) {
        for (int i = 0; i < P; i++) {
            fprintf(fp_coeff, "%.15f\n", h[i]);
        }
        fclose(fp_coeff);
        printf("Filter coefficients saved to: %s\n", coeff_filename);
    } else {
        printf("Warning: Could not create coefficient file.\n");
    }

    // 6. 計算輸出大小
    int num_output_samples = (int)((long long)num_input_samples * L / M);
    short *output_data = (short *)malloc(num_output_samples * 2 * sizeof(short)); 

    printf("Processing... (Target Output: %d samples)\n", num_output_samples);

    // 7. 核心處理迴圈 
    for (int ch = 0; ch < 2; ch++) {
        for (int m = 0; m < num_output_samples; m++) {
            double sum = 0.0;
            for (int k = 0; k < P; k++) {
                long long upsampled_idx = (long long)m * M - k;
                if (upsampled_idx % L == 0) {
                    long long input_idx = upsampled_idx / L;
                    if (input_idx >= 0 && input_idx < num_input_samples) {
                        short sample = input_data[input_idx * 2 + ch];
                        sum += sample * h[k];
                    }
                }
            }
            
            if (sum > 32767) sum = 32767;
            if (sum < -32768) sum = -32768;
            output_data[m * 2 + ch] = (short)sum;
        }
    }

    // 自動產生輸出檔名
    char output_filename[300];
    strcpy(output_filename, input_filename);
    char *dot = strrchr(output_filename, '.');
    if (dot) *dot = '\0';
    strcat(output_filename, "_out.wav");

    // 8. 寫入 WAV 檔案
    write_wav(output_filename, output_data, num_output_samples, 8000);

    // 釋放記憶體
    free(input_data);
    free(output_data);
    free(h);
    
    printf("Done. Saved as: %s\n", output_filename);
    return 0;
}