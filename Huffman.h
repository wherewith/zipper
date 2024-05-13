#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <iostream>
#include <memory>
#include <map>
#include <vector>

struct Node {
    char data;
    unsigned freq;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    Node(char data, unsigned freq) : data(data), freq(freq), left(nullptr), right(nullptr) {}
};

struct MinHeap {
    unsigned size;
    unsigned capacity;
    std::vector<std::unique_ptr<Node>> array;

    MinHeap(unsigned capacity) : size(0), capacity(capacity), array(capacity) {}
};

void swap(std::unique_ptr<Node>& a, std::unique_ptr<Node>& b) {
    std::swap(a, b);
}

void minHeapify(MinHeap& heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap.size && heap.array[left]->freq < heap.array[smallest]->freq)
        smallest = left;
    if (right < heap.size && heap.array[right]->freq < heap.array[smallest]->freq)
        smallest = right;

    if (smallest != idx) {
        swap(heap.array[smallest], heap.array[idx]);
        minHeapify(heap, smallest);
    }
}

Node* extractMin(MinHeap& heap) {
    std::unique_ptr<Node> minNode = std::move(heap.array[0]);
    heap.array[0] = std::move(heap.array[heap.size - 1]);
    heap.size--;
    minHeapify(heap, 0);
    return minNode.release();
}

void insertMinHeap(MinHeap& heap, std::unique_ptr<Node> node) {
    heap.size++;
    int i = heap.size - 1;

    while (i && node->freq < heap.array[(i - 1) / 2]->freq) {
        heap.array[i] = std::move(heap.array[(i - 1) / 2]);
        i = (i - 1) / 2;
    }

    heap.array[i] = std::move(node);
}

void buildMinHeap(MinHeap& heap) {
    int n = heap.size - 1;
    for (int i = (n - 1) / 2; i >= 0; --i)
        minHeapify(heap, i);
}

bool isLeaf(const Node* root) {
    return !root->left && !root->right;
}

MinHeap createAndBuildMinHeap(const char data[], const unsigned freq[], int size) {
    MinHeap heap(size);
    for (int i = 0; i < size; ++i)
        heap.array[i] = std::make_unique<Node>(data[i], freq[i]);

    heap.size = size;
    buildMinHeap(heap);
    return heap;
}

std::unique_ptr<Node> buildHuffmanTree(const char data[], const unsigned freq[], int size) {
    auto heap = createAndBuildMinHeap(data, freq, size);
    while (heap.size > 1) {
        auto left = std::unique_ptr<Node>(extractMin(heap));
        auto right = std::unique_ptr<Node>(extractMin(heap));
        auto top = std::make_unique<Node>('$', left->freq + right->freq);
        top->left = std::move(left);
        top->right = std::move(right);
        insertMinHeap(heap, std::move(top));
    }
    return std::unique_ptr<Node>(extractMin(heap));
}

void generateCodes(const Node* node, const std::string& str, std::map<char, std::string>& huffmanCode) {
    if (!node)
        return;
    if (isLeaf(node))
        huffmanCode[node->data] = str;
    generateCodes(node->left.get(), str + "0", huffmanCode);
    generateCodes(node->right.get(), str + "1", huffmanCode);
}

std::map<char, std::string> HuffmanCodes(const char data[], const unsigned freq[], int size) {
    auto root = buildHuffmanTree(data, freq, size);
    std::map<char, std::string> huffmanCode;
    generateCodes(root.get(), "", huffmanCode);
    return huffmanCode;
}

#endif