/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2004 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/


#ifndef VIGRA_GRADIENT_ENERGY_TENSOR_HXX
#define VIGRA_GRADIENT_ENERGY_TENSOR_HXX

#include <cmath>
#include <functional>
#include "vigra/utilities.hxx"
#include "vigra/array_vector.hxx"
#include "vigra/basicimage.hxx"
#include "vigra/combineimages.hxx"
#include "vigra/numerictraits.hxx"
#include "vigra/convolution.hxx"

namespace vigra {

/** \addtogroup TensorImaging Tensor Image Processing
*/
//@{

/********************************************************/
/*                                                      */
/*                 gradientEnergyTensor                 */
/*                                                      */
/********************************************************/

/** \brief Calculate the gradient energy tensor for a scalar valued image.

    These function calculates the gradient energy tensor (GET operator) as described in
    
    M. Felsberg, U. K&ouml;the: 
    <i>"GET: The Connection Between Monogenic Scale-Space and Gaussian Derivatives"</i>, 
    in: R. Kimmel, N. Sochen, J. Weickert (Eds.): Scale Space and PDE Methods in Computer Vision, 
    Proc. of Scale-Space 2005, Lecture Notes in Computer Science 3459, pp. 192-203, Heidelberg: Springer, 2005.
    
    U. K&ouml;the, M. Felsberg: 
    <i>"Riesz-Transforms Versus Derivatives: On the Relationship Between the Boundary Tensor and the Energy Tensor"</i>, 
    in: ditto, pp. 179-191.

    with the given filters: The derivative filter \a derivKernel is applied to the appropriate image dimensions 
    in turn (see the papers above for details), and the other dimension is smoothed with \a smoothKernel. 
    The kernels can be as small as 3x1, e.g. [0.5, 0, -0.5] and [3.0/16.0, 10.0/16.0, 3.0/16.0] respectively.  
    The output image must have 3 bands which will hold the
    tensor components in the order t11, t12 (== t21), t22. The signs of the output are adjusted for a right-handed
    coordinate system. Thus, orientations derived from the tensor will be in counter-clockwise (mathematically positive)
    order, with the x-axis at zero degrees (this is the standard in all VIGRA functions that deal with orientation).
    
    <b> Declarations:</b>

    pass arguments explicitly:
    \code
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
                  class DestIterator, class DestAccessor>
        void gradientEnergyTensor(SrcIterator supperleft, SrcIterator slowerright, SrcAccessor src,
                                  DestIterator dupperleft, DestAccessor dest,
                                  Kernel1D<double> const & derivKernel, Kernel1D<double> const & smoothKernel);
    }
    \endcode

    use argument objects in conjunction with \ref ArgumentObjectFactories:
    \code
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
                  class DestIterator, class DestAccessor>
        void gradientEnergyTensor(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                                  pair<DestIterator, DestAccessor> dest,
                                  Kernel1D<double> const & derivKernel, Kernel1D<double> const & smoothKernel);
    }
    \endcode

    <b> Usage:</b>

    <b>\#include</b> "<a href="gradient_energy_tensor_8hxx-source.html">vigra/gradient_energy_tensor.hxx</a>"

    \code
    FImage img(w,h);
    FVector3Image get(w,h);
    Kernel1D<double> grad, smooth;
    grad.initGaussianDerivative(0.7, 1);
    smooth.initGaussian(0.7);
    ...
    gradientEnergyTensor(srcImageRange(img), destImage(get), grad, smooth);
    \endcode

*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void gradientEnergyTensor(SrcIterator supperleft, SrcIterator slowerright, SrcAccessor src,
                          DestIterator dupperleft, DestAccessor dest,
                          Kernel1D<double> const & derivKernel, Kernel1D<double> const & smoothKernel)
{
    vigra_precondition(dest.size(dupperleft) == 3,
                       "gradientEnergyTensor(): output image must have 3 bands.");

    int w = slowerright.x - supperleft.x;
    int h = slowerright.y - supperleft.y;
    
    typedef typename 
       NumericTraits<typename SrcAccessor::value_type>::RealPromote TmpType;
    typedef BasicImage<TmpType> TmpImage;    
    TmpImage gx(w, h), gy(w, h), 
             gxx(w, h), gxy(w, h), gyy(w, h), 
             laplace(w, h), gx3(w, h), gy3(w, h);
    
    convolveImage(srcIterRange(supperleft, slowerright, src), destImage(gx), 
                  derivKernel, smoothKernel);
    convolveImage(srcIterRange(supperleft, slowerright, src), destImage(gy), 
                  smoothKernel, derivKernel);
    convolveImage(srcImageRange(gx), destImage(gxx), 
                  derivKernel, smoothKernel);
    convolveImage(srcImageRange(gx), destImage(gxy), 
                  smoothKernel, derivKernel);
    convolveImage(srcImageRange(gy), destImage(gyy), 
                  smoothKernel, derivKernel);
    combineTwoImages(srcImageRange(gxx), srcImage(gyy), destImage(laplace), 
                     std::plus<TmpType>());
    convolveImage(srcImageRange(laplace), destImage(gx3), 
                  derivKernel, smoothKernel);
    convolveImage(srcImageRange(laplace), destImage(gy3), 
                  smoothKernel, derivKernel);
    typename TmpImage::iterator gxi  = gx.begin(),
                                gyi  = gy.begin(),
                                gxxi = gxx.begin(),
                                gxyi = gxy.begin(),
                                gyyi = gyy.begin(),
                                gx3i = gx3.begin(),
                                gy3i = gy3.begin();
    for(int y = 0; y < h; ++y, ++dupperleft.y)
    {
        typename DestIterator::row_iterator d = dupperleft.rowIterator(); 
        for(int x = 0; x < w; ++x, ++d, ++gxi, ++gyi, ++gxxi, ++gxyi, ++gyyi, ++gx3i, ++gy3i)
        {
            dest.setComponent(sq(*gxxi) + sq(*gxyi) - *gxi * *gx3i, d, 0);
            dest.setComponent(- *gxyi * (*gxxi + *gyyi) + 0.5 * (*gxi * *gy3i + *gyi * *gx3i), d, 1);
            dest.setComponent(sq(*gxyi) + sq(*gyyi) - *gyi * *gy3i, d, 2);
        }
    }
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline
void gradientEnergyTensor(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                          pair<DestIterator, DestAccessor> dest,
                          Kernel1D<double> const & derivKernel, Kernel1D<double> const & smoothKernel)
{
    gradientEnergyTensor(src.first, src.second, src.third,
                         dest.first, dest.second, derivKernel, smoothKernel);
}

//@}

} // namespace vigra

#endif // VIGRA_GRADIENT_ENERGY_TENSOR_HXX