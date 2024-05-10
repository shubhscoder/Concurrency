#include <iostream>
#include <vector>

class ByteIO {
private:
    // Position in the current blocl
    size_t cur_blk_num;
    size_t cur_blk_pos;
    size_t blk_sz;
    std::vector<char> cur_blk;

    int bSeek(size_t blockNum) {
        // Implementation of block seek goes here
        return 0; // Placeholder
    }

    int bWrite(void *blockPtr) {
        // Implementation of block write goes here
        return 0; // Placeholder
    }

    int bRead(void *blockPtr) {
        // Implementation of block read goes here
        return 0; // Placeholder
    }

    int bBlockSize() {
        // Implementation of block size retrieval goes here
        return 8; // Placeholder
    }

    public:
    
    ByteIO() :
    cur_blk_num(0),
    cur_blk_pos(0),
    blk_sz(bBlockSize()),
    cur_blk(std::vector<char>(blk_sz)) {}

    int write(void* data, size_t len) {
        size_t bytes_rem = len;
        char* data_ptr = static_cast<char*>(data);
        while (bytes_rem > 0) {
            size_t to_write = std::min(bytes_rem, blk_sz - cur_blk_pos);
            memcpy(cur_blk.data() + cur_blk_pos, data_ptr, to_write);
            bytes_rem -= to_write;
            data_ptr += to_write;
            cur_blk_pos += to_write;
            if (cur_blk_pos == blk_sz) {
                cur_blk_pos = 0;
                cur_blk_num++;
                if (bSeek(cur_blk_num) != 0) {
                    return -1;
                }
            }
        }

        return 0;
    }

    int read(void* data, size_t len) {
        size_t bytes_rem = len;
        char* data_ptr = static_cast<char*>(data);
        if (bRead(cur_blk.data()) != 0) {
            return -1;
        }

        while (bytes_rem > 0) {
            size_t to_read = std::min(bytes_rem, blk_sz - cur_blk_pos);
            memcpy(data_ptr, cur_blk.data() + cur_blk_pos, to_read);
            bytes_rem -= to_read;
            data_ptr += to_read;
            cur_blk_pos += to_read;
            if (cur_blk_pos == blk_sz) {
                cur_blk_pos = 0;
                cur_blk_num++;
                if (bSeek(cur_blk_num) != 0) {
                    return -1;
                }

                if (bRead(cur_blk.data()) != 0) {
                    return -1;
                }
            }
        } 

        return 0;
    }

    int seek(size_t location) {
        assert(blk_sz != 0);
        size_t blk_num = location / blk_sz;
        size_t blk_pos = location % blk_sz;
        if (bSeek(blk_num) != 0) {
            return -1;
        }

        cur_blk_num = blk_num;
        cur_blk_pos = blk_pos;

        return 0;
    }
};

int main() {

}