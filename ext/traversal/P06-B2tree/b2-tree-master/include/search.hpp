#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>

template<class T>
size_t binarySearch(T x, const T* arr, size_t lower, size_t upper) {
    do {
        size_t mid = ((upper - lower) / 2) + lower;
        if (x < arr[mid]) {
            upper = mid;
        } else if (x > arr[mid]) {
            lower = mid + 1;
        } else {
            return mid;
        }
    } while (lower < upper);
    return lower;
}

template<class T>
size_t binarySearch(T x, const T* arr, size_t n) {
    return binarySearch(x, arr, 0, n);
}

template<class T>
size_t guidedExponentialSearch(T x, const T* arr, size_t n, float hint) {
    size_t lower = 0, upper = n;
    size_t start = static_cast<size_t>((n - 1)*hint);

    auto value = arr[start];
    if (value == x) {
        return start;
    } else if (value > x) {
        upper = start;
        long current = start, i = 1;
        while (current > 0 && arr[current] >= x) {
            current = std::max(0l, current - i);
            i = i<<1;
        }
        lower = current;
    } else {
        lower = start;
        long current = start, i = 1, count = n;
        while (current < count && arr[current] <= x) {
            current = std::min(count, current + i);
            i = i<<1;
        }
        upper = current;
    }

    return binarySearch(x, arr, lower, upper);
}

// source: https://en.wikipedia.org/wiki/Interpolation_search
template <typename T>
int interpolation_search(T arr[], int size, T key)
{
    int low = 0;
    int high = size - 1;
    int mid;

    while ((arr[high] != arr[low]) && (key >= arr[low]) && (key <= arr[high])) {
        mid = low + ((key - arr[low]) * (high - low) / (arr[high] - arr[low]));

        if (arr[mid] < key)
            low = mid + 1;
        else if (key < arr[mid])
            high = mid - 1;
        else
            return mid;
    }

    if (key == arr[low])
        return low ;
    else
        return -1;
}
