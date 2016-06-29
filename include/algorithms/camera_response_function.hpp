/*

PICCANTE
The hottest HDR imaging library!
http://vcg.isti.cnr.it/piccante

Copyright (C) 2014
Visual Computing Laboratory - ISTI CNR
http://vcg.isti.cnr.it
First author: Francesco Banterle

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef PIC_ALGORITHMS_CAMERA_RESPONSE_FUNCTION_HPP
#define PIC_ALGORITHMS_CAMERA_RESPONSE_FUNCTION_HPP

#include "image.hpp"
#include "point_samplers/sampler_random.hpp"
#include "histogram.hpp"
#include "filtering/filter_mean.hpp"

#ifndef PIC_DISABLE_EIGEN
    #include "externals/Eigen/SVD"
#endif

namespace pic {

/**
 * @brief The CRF_WEIGHT enum
 */
enum CRF_WEIGHT {CW_ALL, CW_HAT, CW_DEB97, CW_DEB97p01, CW_GAUSS};

/**
 * @brief WeightFunction computes weight functions for x in [0,1].
 * @param x is an input value in [0, 1].
 * @param type is the type of the function.
 * @return It returns a weight for x.
 */
inline float WeightFunction(float x, CRF_WEIGHT type)
{
    switch(type) {

    case CW_GAUSS: {
        float sigma = 0.5f;
        float mu = 0.5f;
        float sigma_sq_2 = 2.0f * (sigma * sigma);
        float x_mu = (x - mu);
        return expf(-4.0f * (x_mu * x_mu) / sigma_sq_2);
    }
    break;

    case CW_ROBERTSON: {
        float sigma = 0.5f;
        float mu = 0.5f;
        float mu_sq = mu * mu;
        float sigma_sq_2 = 2.0f * (sigma * sigma);

        float x_mu = (x - mu);

        float y =  expf(-4.0f * (x_mu * x_mu) / sigma_sq_2);

        float shift_val = expf(-4.0f * mu_sq / sigma_sq_2);
        float scale_val = expf(0.0f);

        y = (y - shift_val) / (scale_val - shift_val);
        return  CLAMPi(y, 0.0f, 1.0f);
    }
    break;


    case CW_HAT: {
        float val = (2.0f * x - 1.0f);
        float val_squared = val * val;
        float val_quartic = val_squared * val_squared;
        return (1.0f - val_quartic * val_quartic * val_quartic);
    }

    case CW_DEB97: {
        float Zmin = 0.0f;
        float Zmax = 1.0f;
        float tr = (Zmin + Zmax) / 2.0f;

        if(x <= tr) {
            return x - Zmin;
        } else {
            return Zmax - x;
        }
    }
    break;

    case CW_DEB97p01: {
        float Zmin = 0.01f;
        float Zmax = 0.99f;
        float tr = (Zmin + Zmax) / 2.0f;

        if(x <= tr) {
            return x - Zmin;
        } else {
            return Zmax - x;
        }
    }

    }

    return 1.0f;
}

enum IMG_LIN {IL_LIN, IL_2_2, IL_LUT_8_BIT};

/**
 * @brief The CameraResponseFunction class
 */
class CameraResponseFunction
{
protected:

    /**
    * \brief gsolve computes the inverse CRF of a camera.
    */
    float *gsolve(unsigned char *samples, float *log_exposure, float lambda,
                  int nSamples, int nExposure)
    {
		#ifndef PIC_DISABLE_EIGEN

        int n = 256;
        int rows = nSamples * nExposure + n + 1;
        int cols = n + nSamples;

        #ifdef PIC_DEBUG
            printf("Matrix size: (%d, %d)\n", rows, cols);
        #endif

        Eigen::MatrixXf A = Eigen::MatrixXf::Zero(rows, cols);
        Eigen::VectorXf b = Eigen::VectorXf::Zero(rows);

        int k = 0;

        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nExposure; j++) {
                int tmp = samples[i * nExposure + j];

                float w_ij = w[tmp];

                A.coeffRef(k, tmp)   =  w_ij;
                A.coeffRef(k, n + i) = -w_ij;
                
                b[k]                 =  w_ij * log_exposure[j];

                k++;
            }
        }

        A.coeffRef(k, 128) = 1.0f;
        k++;

        //Smoothness term
        for(int i = 0; i < (n - 2); i++) {
            float w_l = lambda * w[i + 1];
            A.coeffRef(k, i)     =         w_l;
            A.coeffRef(k, i + 1) = -2.0f * w_l;
            A.coeffRef(k, i + 2) =         w_l;
            k++;
        }

        //Solving the linear system
        Eigen::JacobiSVD< Eigen::MatrixXf > svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);

        Eigen::VectorXf x = svd.solve(b);

        float *ret = new float[n];

        for(int i = 0; i < n; i++) {
            ret[i] = expf(x[i]);
        }
		#else
            float *ret = NULL;
        #endif

        return ret;
    }

    /**
    * \brief This function creates a low resolution version of the stack using Grossberg and Nayar sampling.
    * \param stack is a stack of Image* at different exposures
    * \param nSamples output number of samples
    * \return samples an array of unsigned char values which is the low resolution stack
    */
    unsigned char *subSampleGrossberg(std::vector<Image *> stack, int nSamples = 100)
    {        
        if(stack.size() < 1) {
            return NULL;
        }
     
        if(nSamples < 1) {
            nSamples = 256;
        }

        int channels  = stack[0]->channels;
        unsigned int exposures = stack.size();
        
        Histogram *h = new Histogram[exposures * channels];
        
        int c = 0;
        
        #ifdef PIC_DEBUG
            printf("Computing histograms...");
        #endif
        
        for(int j = 0; j < channels; j++) {
            for(unsigned int i = 0; i < exposures; i++) {
                h[c].Calculate(stack[i], VS_LDR, 256, j);
                h[c].cumulativef(true);
                c++;
            }
        }

        #ifdef PIC_DEBUG
            printf("Ok\n");
        #endif        
        
        unsigned char *samples = new unsigned char[nSamples * channels * exposures];
        
        #ifdef PIC_DEBUG
            printf("Sampling...");
        #endif
        
        c = 0;
        for(int k = 0; k < channels; k++) {
            for(int i = 0; i < nSamples; i++) {

                float u = float(i) / float(nSamples);

                for(unsigned int j = 0; j < exposures; j++) {
        
                    int ind = k * exposures + j;

                    float *bin_c = h[ind].getCumulativef();
                    
                    float *ptr = std::upper_bound(&bin_c[0], &bin_c[255], u);
                    int offset = CLAMPi((int)(ptr - bin_c), 0, 255);

                    samples[c] = offset;
                    c++;
                }
            }
        }

        #ifdef PIC_DEBUG
            printf("Ok\n");
        #endif

        return samples;
    }
    
    /**
     * @brief subSampleSpatial creates a low resolution version of the stack.
    * \param stack is a stack of Image* at different exposures
    * \param nSamples output number of samples
    * \return samples an array of unsigned char values which is the low resolution stack
     * @return
     */
    unsigned char *subSampleSpatial(std::vector<Image *> stack, int &nSamples)
    {
        if(stack.size() < 1) {
            return NULL;
        }
        
        int width    = stack[0]->width;
        int height   = stack[0]->height;
        int channels = stack[0]->channels;
        
        Vec<2, int> vec(width, height);
        
        RandomSampler<2> *p2Ds = new RandomSampler<2>(ST_BRIDSON, vec, nSamples, 1, 0);
        
        #ifdef PIC_DEBUG
            int oldNSamples = nSamples;
        #endif

        nSamples = p2Ds->getSamplesPerLevel(0);
        
        #ifdef PIC_DEBUG
            printf("--subSample samples: %d \t \t old samples: %d\n", nSamples, oldNSamples);
        #endif
        
        int c = 0;
        
        unsigned char *samples = new unsigned char[nSamples * channels * stack.size()];
        
        for(int k = 0; k < channels; k++) {
            for(int i = 0; i <nSamples; i++) {

                int x, y;
                p2Ds->getSampleAt(0, i, x, y);              
                
                for(unsigned int j = 0; j < stack.size(); j++) {
                    int converted = int((*stack[j])(x, y)[k] * 255.0f);
                    samples[c] = converted;
                    c++;
                }
            }
        }
        
        return samples;
    }

    /**
     * @brief Destroy frees memory.
     */
    void Destroy()
    {
        for(unsigned int i=0; i<icrf.size(); i++) {
            if(icrf[i] != NULL) {
                delete[] icrf[i];
            }
        }
    }

    CRF_WEIGHT              type;
    float                   w[256];

public:

    std::vector<float *>    icrf;
    
    /**
     * @brief CameraResponseFunction
     */
    CameraResponseFunction()
    {
        
    }
    
    /**
     * @brief CameraResponseFunction
     * @param stack
     * @param exposure
     * @param type
     * @param nSamples
     * @param lambda
     */
    CameraResponseFunction(ImageVec stack, CRF_WEIGHT type = CW_DEB97, int nSamples = 100, float lambda = 10.0f)
    {
        DebevecMalik(stack, type, nSamples, lambda);
    }

    ~CameraResponseFunction()
    {
        //Destroy();
    }

    /**
     * @brief FromRAWJPEG computes the CRF by exploiting the couple RAW/JPEG from cameras.
     * @param img_raw is a RAW image.
     * @param img_jpg is a JPEG compressed image.
     * @param filteringSize
     */
    void FromRAWJPEG(Image *img_raw, Image *img_jpg, int filteringSize = 11)
    {
        if((img_raw == NULL) || (img_jpg == NULL))
            return;

        if(!img_raw->SimilarType(img_jpg))
            return;
        
        icrf.clear();

        this->type = CW_ALL;

        int width    = img_raw->width;
        int height   = img_raw->height;
        int channels = img_raw->channels;

        int crf_size = 256 * 256 * channels;
        unsigned int *crf = new unsigned int[crf_size];

        for(int i=0;i<crf_size;i++) {
            crf[i] = 0;
        }
               
        for(int i=0;i<height;i++) {
            for(int j=0;j<width;j++) {

                float *data_raw = (*img_raw)(j, i);
                float *data_jpg = (*img_jpg)(j, i);               

                for(int k=0;k<channels;k++) {
                    int i_raw = CLAMPi(int(255.0f * data_raw[k]), 0, 255);
                    int i_jpg = CLAMPi(int(255.0f * data_jpg[k]), 0, 255);

                    int addr = (i_raw * 256 + i_jpg ) * channels;

                    crf[addr + k ]++;
                }
            }
        }
       
        //computing the result
        std::vector< int > coords;

        for(int k=0;k<channels;k++) {

            float *ret_c = new float[256];

            for(int j=0;j<256;j++) {
                coords.clear();

                for(int i=0;i<256;i++) {

                    int addr = (i * 256 + j ) * channels + k;

                    if(crf[addr] > 0) {
                        coords.push_back(i);                        
                    }

                }

                if(!coords.empty()) {//getting the median value
                    std::sort (coords.begin(), coords.end());  
                    ret_c[j] = float(coords[coords.size() >> 1]) / 255.0f;
                }
            }
            
            if(filteringSize > 0) {
                Image toBeFiltered(1, 256, 1, 1, ret_c);

                Image *filtered = FilterMean::Execute(&toBeFiltered, NULL, filteringSize);
                
                icrf.push_back(filtered->data);

            } else {
                icrf.push_back(ret_c);
            }
        }
    }

    /**
     * @brief DebevecMalik computes the CRF of a camera using multiple exposures value following Debevec and Malik
    1997's method.
     * @param stack
     * @param exposure
     * @param type
     * @param nSamples
     * @param lambda
     */
    void DebevecMalik(ImageVec stack, CRF_WEIGHT type = CW_DEB97, int nSamples = 256, float lambda = 20.0f)
    {
        if(stack.empty()) {
            return;
        }

        if(nSamples < 1) {
            nSamples = 256;
        }

        icrf.clear();

        this->type = type;

        //Subsampling the image stack
        unsigned char *samples = subSampleGrossberg(stack, nSamples);
        
        //Computing CRF using Debevec and Malik
        int channels = stack[0]->channels;
                    
        //precomputing the weight function
        for(int i = 0; i < 256; i++) {
            w[i] = WeightFunction(float(i) / 255.0f, type);
        }

        unsigned int nExposure = stack.size();

        //log domain exposure time
        float *log_exposure = new float[nExposure];
        for(unsigned int i = 0; i < nExposure; i++) {
            log_exposure[i] = logf(stack[i]->exposure);
        }

        int stride = nSamples * nExposure;

        #ifdef PIC_DEBUG
            printf("nSamples: %d\n", nSamples);
        #endif

        for(int i = 0; i < channels; i++) {
            float *icrf_channel = gsolve(&samples[i * stride], log_exposure, lambda, nSamples,
                                        nExposure);

            //Wrapping into an Image for normalization
            Image img(1, 256, 1, 1, icrf_channel);

            float *max_val = img.getMaxVal(NULL, NULL);
            if(max_val[0] > 0.0f) {
                img /= max_val[0];
            }

            icrf.push_back(icrf_channel);
        }
        
        delete[] log_exposure;
        delete[] samples;
    }

    /**
     * @brief MitsunagaNayar
     * @param stack
     * @param polynomial_degree
     * @param nSamples
     */
    void MitsunagaNayar(ImageVec stack, int polynomial_degree = 3, int nSamples = 100)
    {

    }

    /**
     * @brief RemoveCRF removes a camera resposnse function to a value.
     * @param x is an intensity value in [0,1].
     * @param type describes how x values are encoded.
     * @param icrf is the inverse camera response function stored as
     * an array of values of 256 elements.
     * @return It returns x in the linear domain.
     */
    static inline float RemoveCRF(float x, IMG_LIN type, float *icrf = NULL)
    {
        switch(type) {
            case IL_LIN: {
                return x;
            }
            break;

            case IL_LUT_8_BIT: {
                int index =  CLAMP(int(lround(x * 255.0f)), 256);
                return icrf[index];
            }
            break;

            case IL_2_2: {
                return powf(x, 2.2f);
            }
            break;

        }

        return x;
    }

    /**
     * @brief ApplyCRF
     * @param x a value in [0, 1]
     * @param type
     * @param icrf
     * @return
     */
    static inline float ApplyCRF(float x, IMG_LIN type, float *icrf = NULL)
    {        
        switch(type) {
            case IL_LIN: {
                return x;
            }
            break;

            case IL_LUT_8_BIT: {
                float *ptr = std::lower_bound(&icrf[0], &icrf[255], x);
                int offset = CLAMPi((int)(ptr - icrf), 0, 255);

                return float(offset) / 255.0f;
            }
            break;

            case IL_2_2: {
                return powf(x, 1.0f / 2.2f);
            }
            break;

        }

        return x;
    }

    /**
     * @brief Linearize
     * @param img
     * @param type
     */
    void Linearize(Image *img, IMG_LIN type = IL_LUT_8_BIT)
    {
        if(img == NULL) {
            return;
        }

        if(!img->isValid()) {
            return;
        }

        if(type == IL_LUT_8_BIT && icrf.size() != img->channels) {
        #ifdef PIC_DEBUG
            printf("Warning: img cannot be linearized.\n");
        #endif
            return;
        }

        for(int i=0; i<img->size(); i += img->channels) {
            for(int j=0; j<img->channels; j++) {
                int ind = i + j;
                img->data[ind] = RemoveCRF(img->data[ind] , type, icrf[j]);
            }
        }
    }

    /**
     * @brief ApplyCRF
     * @param img
     * @param type
     */
    void ApplyCRF(Image *img, IMG_LIN type = IL_LUT_8_BIT)
    {
        if(img == NULL) {
            return;
        }

        if(!img->isValid()) {
            return;
        }

        if(type == IL_LUT_8_BIT && icrf.size() != img->channels) {
        #ifdef PIC_DEBUG
            printf("Warning: CRF cannot be applied to the image.\n");
        #endif
            return;
        }

        for(int i=0; i<img->size(); i += img->channels) {
            for(int j=0; j<img->channels; j++) {
                int ind = i + j;
                img->data[ind] = ApplyCRF(img->data[ind] , type, icrf[j]);
            }
        }
    }
};

} // end namespace pic

#endif /* PIC_ALGORITHMS_CAMERA_RESPONSE_FUNCTION_HPP */

