# Analisa Fibonacci dengan OpenMP

## Hasil Eksekusi

Berdasarkan hasil running program, menunjukkan bahwa:
- **Sequential** memiliki waktu eksekusi yang **lebih cepat**
- **Parallel (OpenMP)** memiliki waktu eksekusi yang **lebih lambat**

## Mengapa Multi-threading Lebih Lambat?

### 1. **Overhead Task Creation**
Setiap rekursi pada implementasi parallel membuat task baru menggunakan `#pragma omp task`. Untuk fibonacci(40), ini menciptakan **jutaan task**. Overhead pembuatan, scheduling, dan sinkronisasi task ini sangat besar dan melebihi keuntungan dari paralelisasi.

### 2. **Task Granularity Terlalu Kecil**
Operasi fibonacci untuk angka kecil (n < 10) sangat cepat (mikrodetik). Overhead pembuatan thread untuk task sekecil ini jauh lebih besar daripada komputasi aktualnya. Ini disebut "fine-grained parallelism" yang tidak efisien.

### 3. **Overhead Sinkronisasi (taskwait)**
Setiap level rekursi harus menunggu (`#pragma omp taskwait`) task child selesai sebelum melanjutkan. Dengan struktur rekursif yang dalam, overhead sinkronisasi ini terjadi jutaan kali.

### 4. **Memory Contention dan Cache Misses**
Multiple thread mengakses memori secara bersamaan menyebabkan:
- Cache coherency overhead
- False sharing
- Memory bandwidth contention
- Lebih banyak cache misses

### 5. **CPU Time vs Wall Time**
CPU Time pada parallel lebih tinggi karena menghitung **total waktu semua thread**:
- CPU Time = waktu thread 1 + waktu thread 2 + ... + waktu thread n
- Ini menunjukkan total resource CPU yang terpakai
- Wall Time yang lebih tinggi menunjukkan overhead koordinasi thread

## Kapan OpenMP Menguntungkan?

OpenMP akan lebih cepat jika:

1. **Coarse-grained parallelism**: Hanya membuat task untuk n yang cukup besar
   ```c
   if (n > 30) {
       #pragma omp task
       x = fib(n-1);
   } else {
       x = fib_sequential(n-1);
   }
   ```

2. **Workload yang lebih berat**: Komputasi per task jauh lebih besar dari overhead

3. **Independen computation**: Tidak banyak memerlukan sinkronisasi

4. **Loop parallelization**: Lebih cocok untuk `#pragma omp parallel for` daripada recursive task

## Kesimpulan

Untuk algoritma **rekursif dengan banyak small tasks** seperti fibonacci, sequential lebih efisien. Multi-threading memerlukan overhead yang signifikan dan hanya menguntungkan untuk:
- Komputasi yang sangat berat per task
- Data parallelism (loop besar)
- Independent tasks dengan minimal synchronization
