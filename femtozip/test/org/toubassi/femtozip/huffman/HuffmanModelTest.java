package org.toubassi.femtozip.huffman;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Random;

import junit.framework.Assert;

import org.junit.Test;
import org.toubassi.femtozip.encoding.arithcoding.FrequencyCodeModel;


public class HuffmanModelTest {
    
    @Test
    public void testSimpleHuffman() throws IOException {
        String data = "a man a plan a canal panama";
        testString(data, true);
        testString(data, false);
    }
    
    public void testString(String string, boolean allSymbolsSampled) throws IOException {
        byte[] dataBytes = string.getBytes("UTF-8");
        int[] data = new int[dataBytes.length];
        for (int i = 0, count = dataBytes.length; i < count; i++) {
            data[i] = ((int)dataBytes[i]) & 0xff;
        }
        int[] histogram = FrequencyCodeModel.computeHistogramWithEOFSymbol(dataBytes);
        
        HuffmanModel model = new HuffmanModel(histogram, allSymbolsSampled);
        
        testDataWithModel(data, model);
    }

    private void testDataWithModel(int[] data, HuffmanModel model) throws IOException {
        ByteArrayOutputStream bytesOut = new ByteArrayOutputStream();
        Encoder encoder = new Encoder(model, bytesOut);
        
        for (int i = 0, count = data.length; i < count; i++) {
            encoder.encodeSymbol(data[i]);
        }
        encoder.close();
        
        byte[] compressedBytes = bytesOut.toByteArray();
        Decoder decoder = new Decoder(model, new ByteArrayInputStream(compressedBytes));
        ArrayList<Integer> decompressed = new ArrayList<Integer>();
        int symbol;
        while ((symbol = decoder.decodeSymbol()) != -1) {
            decompressed.add(symbol);
        }
        
        Assert.assertEquals(data.length, decompressed.size());
        for (int i = 0, count = data.length; i < count; i++) {
            Assert.assertEquals(data[i], decompressed.get(i).intValue());
        }
    }
    
    @Test
    public void testNestedDecodingTables() throws IOException {
        Random random = new Random(1234567);
        
        for (int dataSize = 2; dataSize < 2000; dataSize++) {
            int[] histogram = new int[dataSize];
            for (int i = 0, count = histogram.length; i < count; i++) {
                histogram[i] = 20 + random.nextInt(10);
            }
            
            HuffmanModel model = new HuffmanModel(histogram, false);

            int[] data = new int[histogram.length];
            for (int i = 0, count = data.length; i < count; i++) {
                data[i] = random.nextInt(histogram.length - 1); // -1 so we don't emit EOF mid stream!
            }
            
            testDataWithModel(data, model);
        }
    }

}
