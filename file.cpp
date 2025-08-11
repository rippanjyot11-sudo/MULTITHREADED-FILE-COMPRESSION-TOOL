#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>

struct Chunk {
    std::string compressed;
    char firstChar;
    int firstCount;
    char lastChar;
    int lastCount;
};

std::mutex mtx;

Chunk compressChunk(const std::string &data, int start, int end) {
    Chunk result;
    result.compressed = "";

    if (start >= end) return result;

    char current = data[start];
    int count = 1;

    for (int i = start + 1; i < end; ++i) {
        if (data[i] == current) {
            count++;
        } else {
            result.compressed += current + std::to_string(count);
            current = data[i];
            count = 1;
        }
    }

    result.firstChar = data[start];
    result.lastChar = current;
    result.firstCount = 0;
    result.lastCount = count;

    // Re-scan from start for firstCount
    current = data[start];
    for (int i = start; i < end; ++i) {
        if (data[i] == current) result.firstCount++;
        else break;
    }

    // Add last segment to compressed string
    result.compressed += current + std::to_string(count);

    return result;
}

std::string mergeChunks(const std::vector<Chunk> &chunks) {
    std::string finalCompressed = "";

    for (size_t i = 0; i < chunks.size(); ++i) {
        if (i == 0) {
            finalCompressed += chunks[i].compressed;
        } else {
            // If boundary characters match
            if (chunks[i - 1].lastChar == chunks[i].firstChar) {
                // Remove last part from previous chunk
                size_t pos = finalCompressed.rfind(chunks[i - 1].lastChar + std::to_string(chunks[i - 1].lastCount));
                if (pos != std::string::npos) {
                    finalCompressed.erase(pos);
                    int combined = chunks[i - 1].lastCount + chunks[i].firstCount;
                    finalCompressed += chunks[i - 1].lastChar + std::to_string(combined);
                }

                // Append rest of this chunk, skipping duplicate count at start
                size_t skip = (chunks[i].firstChar + std::to_string(chunks[i].firstCount)).size();
                finalCompressed += chunks[i].compressed.substr(skip);
            } else {
                finalCompressed += chunks[i].compressed;
            }
        }
    }
    return finalCompressed;
}

int main() {
    std::ifstream inputFile("test.txt");
    if (!inputFile.is_open()) {
        std::cerr << "❌ Failed to open input file." << std::endl;
        return 1;
    }

    std::string data((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();

    const int numThreads = 4;
    int chunkSize = data.size() / numThreads;
    std::vector<Chunk> results(numThreads);
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        int start = i * chunkSize;
        int end = (i == numThreads - 1) ? data.size() : start + chunkSize;
        threads.emplace_back([&results, &data, i, start, end]() {
            results[i] = compressChunk(data, start, end);
        });
    }

    for (auto &t : threads) t.join();

    std::string finalOutput = mergeChunks(results);

    std::ofstream outputFile("compressed.txt");
    outputFile << finalOutput;
    outputFile.close();

    std::cout << "✅ Compression done. Output saved to compressed.txt\n";
    return 0;
}
