/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Julian Santander
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once
#include <vector>

/**
 * Implementantion of a ring buffer.
 * Data is contained in a vector dynamically allocated.
 */
template<typename T>
class RingBuffer {
public:
    /**
     * @brief RingBuffer
     * @param max maximum number of elements that can be contained in the ring buffer
     */
    RingBuffer(std::size_t max = 0):mData(max),mRead(0),mWrite(0),mFull(false) { }

    /**
     * @brief Resets the ring_buffer
     * @param max maximum number of elements that can be contained in the ring buffer.
     */
    void reset(std::size_t max = 0) {
        mData = std::vector<T>(max);
        mRead = 0;
        mWrite = 0;
        mFull = false;
    }

    /**
     * @brief Current number of elements contained in the ring buffer
     * @return
     */
    std::size_t size() const {
        std::size_t size = capacity();
        if(!mFull) {
            if(mWrite >= mRead) {
                size = mWrite - mRead;
            } else {
                size = capacity() + mWrite - mRead;
            }
        }
        return size;
    }

    /**
     * @brief whether the ring buffer is full
     * @return
     */
    bool isFull() const { return mFull; }

    /**
     * @brief whether the ring buffer is empty.
     * @return
     */
    bool isEmpty() const { return !isFull() && (mRead == mWrite); }

    /**
     * @brief Maximum number of elements in the ring buffer
     * @return
     */
    std::size_t capacity() const { return mData.size(); }

    /**
     * @brief Adds a single value
     * @param v the value to add
     */
    void put(const T &v) {
        mData[mWrite] = v;
        forward();
    }

    /**
     * @brief Reads a single value
     * @param v the value read
     * @return true if the value was read
     */
    bool get(T&v) {
        if(!isEmpty()) {
            v = mData[mRead];
            backward();
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Adds a multiple consecutive values
     * @param v pointer to the consecutive values
     * @param count number of consecutive values.
     */
    void put(const T *v, std::size_t count) {
        std::size_t avail = mWrite - capacity();
        std::size_t to_copy = std::min(count,avail);
        memcpy(mData.data() + mWrite,v, to_copy*sizeof(T));
        forward(to_copy);
        if(to_copy < count) {
            put(v+to_copy,count - to_copy);
        }
    }

    /**
     * @brief Reads multiple values
     * @param v pointer to the memory wher ethe values will be written
     * @param count Maximum available size in the memory area
     * @return actual number of elements read.
     */
    std::size_t get(T *v, std::size_t count) {
        std::size_t avail = 0;
        if(mRead < mWrite) {
            avail = mWrite - mRead;
        } else {
            avail = mRead - capacity();
        }
        std::size_t to_copy = std::min(count, avail);
        memcpy(v, mData.data() + mRead, to_copy * sizeof(T));
        backward(to_copy);
        if((size()>0)&&(count > to_copy)) {
            return to_copy + get(v + to_copy, count - to_copy);
        } else {
            return to_copy;
        }
    }
private:
    void forward() {
        if(isFull()) {
            mRead = (mRead + 1) % capacity();
        }
        mWrite = (mWrite + 1) % capacity();
        mFull = (mRead == mWrite);
    }

    void forward(std::size_t count) {
        for(std::size_t i=0; i<count;i++) {
            forward();
        }
    }

    void backward(std::size_t count) {
        mFull = false;
        mRead = (mRead + count) % capacity();
    }

    std::vector<T> mData;
    std::size_t mRead;  /** offset to reading point */
    std::size_t mWrite; /** offset to writing point */
    bool mFull;
};

