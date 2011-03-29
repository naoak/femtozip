/**
 *   Copyright 2011 Garrick Toubassi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "DataIO.h"
#include "DocumentList.h"
#include "CompressionModel.h"
#include "OptimizingCompressionModel.h"
#include "femtozip.h"

using namespace std;
using namespace femtozip;

class CCallbackDocumentList : public DocumentList {
private:
    int count;
    bool even;
    const char *(*get_callback)(int doc_index, int *doc_len);
    void (*release_callback)(const char *buf);

public:
    CCallbackDocumentList(int size, bool even, const char *(*get_callback)(int doc_index, int *doc_len), void (*release_callback)(const char *buf)) : count(size), even(even), get_callback(get_callback), release_callback(release_callback) {};

    virtual ~CCallbackDocumentList() {}

    virtual int size() {return count / 2 + (even && (count & 1 == 1) ? 1 : 0);}

    virtual const char *get(int i, int& length) {
        i = 2 * i + (even ? 0 : 1);
        return get_callback(i, &length);
    }

    virtual void release(const char *buf) {
        release_callback(buf);
    }
};


#ifdef __cplusplus
extern "C" {
#endif


void *fz_load_model(const char *path) {
    ifstream file(path, ios::in | ios::binary);
    DataInput in(file);
    CompressionModel *model = CompressionModel::loadModel(in);
    file.close();
    return reinterpret_cast<void *>(model);
}

int fz_save_model(void *model, const char *path) {
    CompressionModel *m = reinterpret_cast<CompressionModel*>(model);
    ofstream file(path, ios::out | ios::binary | ios::trunc);
    DataOutput out(file);
    CompressionModel::saveModel(*m, out);
    out.flush();
    file.close();
    return file.fail() ? 1 : 0;
}

void *fz_build_model(int num_docs, const char *(*get_callback)(int doc_index, int *doc_len), void (*release_callback)(const char *buf)) {
    OptimizingCompressionModel *model = new OptimizingCompressionModel();

    CCallbackDocumentList pass1Docs(num_docs, false, get_callback, release_callback);
    CCallbackDocumentList pass2Docs(num_docs, true, get_callback, release_callback);

    model->build(pass1Docs);
    model->optimize(pass2Docs);

    return reinterpret_cast<void *>(model);
}

void fz_release_model(void *model) {
    CompressionModel *m = reinterpret_cast<CompressionModel*>(model);
    delete m;
}

int fz_compress(void *model, const char *source, int source_len, char *dest, int dest_capacity) {
    CompressionModel *m = reinterpret_cast<CompressionModel*>(model);
    ostringstream out;
    m->compress(source, source_len, out);
    string outstr = out.str();
    if (outstr.length() > (size_t)dest_capacity) {
        return -outstr.length();
    }
    memcpy(dest, outstr.c_str(), outstr.length());
    return outstr.length();
}

int fz_decompress(void *model, const char *source, int source_len, char *dest, int dest_capacity) {
    CompressionModel *m = reinterpret_cast<CompressionModel*>(model);
    ostringstream out;
    m->decompress(source, source_len, out);
    string outstr = out.str();
    if (outstr.length() > (size_t)dest_capacity) {
        return -outstr.length();
    }
    memcpy(dest, outstr.c_str(), outstr.length());
    return outstr.length();
}


#ifdef __cplusplus
}
#endif
