// OpenCSG - library for image-based CSG rendering for OpenGL
// Copyright (C) 2002-2025, Florian Kirsch,
// Hasso-Plattner-Institute at the University of Potsdam, Germany
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301 USA.

//
// opencsg.h 
//
// main include file for OpenCSG
//

#ifndef __OpenCSG__opencsg_h__
#define __OpenCSG__opencsg_h__

#include <vector>

/// OpenCSG version. The version number has been introduced with version 1.3.2.
/// Format is Major|Minor|Patch, each of them being 4-bit wide.
#define OPENCSG_VERSION 0x0160
#define OPENCSG_VERSION_STRING "OpenCSG 1.6.0"

namespace OpenCSG {

    enum Operation { Intersection, Subtraction };

    /// Abstract base class for CSG primitives. 
    /// From this class, derive concrete classes to do CSG rendering.
    class Primitive {
    public:
        Primitive(Operation, unsigned int convexity);
        virtual ~Primitive();

        /// Sets the CSG operation.
        /// The operation specifies whether the primitive is intersected
        /// or subtracted.
        void setOperation(Operation);
        /// Returns the CSG operation.
        Operation getOperation() const;

        /// Sets the convexity.
        /// The convexity is the maximum number of front faces of the
        /// primitive at a single position. For example, the convexity
        /// of a sphere is 1, and of a torus is 2. Actually the convexity
        /// is only used in the Goldfeather algorithm. For this algorithm,
        /// a convexity too low may result in rendering errors, a convexity
        /// too high will reduce rendering performance.
        void setConvexity(unsigned int);
        /// Returns the convexity.
        unsigned int getConvexity() const;

        /// Sets the bounding box.
        /// The bounding box of the primitive is given in normalized device 
        /// coordinates, i.e. after modelview and projection transformation.
        /// Setting it is not required, but allows for various performance
        /// optimizations.
        void setBoundingBox(float  minx, float  miny, float  minz,
                            float  maxx, float  maxy, float  maxz);
        /// Returns the bounding box.
        void getBoundingBox(float& minx, float& miny, float& minz,
                            float& maxx, float& maxy, float& maxz) const;

        /// render() is the abstract render method, to be implemented in
        /// derived classes. In the implementation, be sure not to alter
        /// the modelview or projection (glPushMatrix/glPopMatrix might be
        /// a good idea). Also do not alter colors in your render() method:
        /// OpenCSG uses them internally. For best performance, you should
        /// only transmit vertex positions; no normals, tex coords or
        /// whatever else.
        virtual void render() = 0;

    private:
        Operation mOperation;
        unsigned int mConvexity;
        float mMinx, mMiny, mMinz, mMaxx, mMaxy, mMaxz;
    };

    /// The function render() performs CSG rendering. The function initializes 
    /// the z-buffer with the z-values of the CSG product given as array of 
    /// primitives. It does not alter the color buffer, so you have to shade
    /// the primitives using GL_EQUAL depth function afterwards. The content
    /// of the stencil buffer is destroyed when handling concave primitives or 
    /// when using the DepthComplexitySampling strategy.
    ///
    /// render() respects the OpenGL settings of 
    ///   - scissor test (CSG calculating will only occur in the specified region)
    ///   - stencil test, when only convex primitives are used and no layered 
    ///         algorithm is used. Most stenciling ops (increment / decrement 
    ///         / zero / one) will not be useful anyway.
    ///
    /// render() ignores
    ///   - depth test (always GL_LESS)
    ///   - alpha test (used internally)
    ///   - cull face  (used internally to distinguish intersected / subtracted 
    ///         primitives)
    ///
    /// The exact CSG algorithm can be specified using the setOption() function
    /// of which the possible parameters are described below. An overloaded
    /// version of the render() function is provided below, which takes 
    /// some parameters for specifying Algorithm and DepthComplexityAlgorithm 
    /// directly. 
    void render(const std::vector<Primitive*>& primitives);

    /// OpenCSG option for use with setOption() / getOption() below
    enum OptionType {
        AlgorithmSetting          = 0,
        DepthComplexitySetting    = 1,
        OffscreenSetting          = 2,
        DepthBoundsOptimization   = 3,
        CameraOutsideOptimization = 4,
        OptionTypeUnused          = 5
    };

    /// Sets an OpenCSG option.
    /// The option parameter specifies which option to set. The newSetting
    /// is the new setting and should be one of the Algorithm, 
    /// DepthComplexityAlgorithm, or OffscreenType enums below.
    void setOption(OptionType option, int newSetting);
    /// Returns the current setting of the provided option type.
    int  getOption(OptionType option);

    /// The Algorithm specifies the method used for CSG rendering:
    ///   - Goldfeather: This algorithm handles convex and concave primitives.
    ///                  It handles the case that the camera is inside of
    ///                  the CSG model better.
    ///   - SCS        : This algorithm handles only convex primitives.
    ///                  It is usually faster than Goldfeather.
    ///   - Automatic  : This setting currently choses Goldfeather if the
    ///                  primitive vector contains concave primitives, else it
    ///                  choses SCS. Also sets the DepthComplexityAlgorithm
    ///                  (NoDepthComplexitySampling for arrays with few
    ///                  primitives, else OcclusionQuery or at the last resort
    ///                  DepthComplexitySampling). This setting is the default.
    ///   - AlgorithmUnused: For use with setOption, this value is invalid.
    ///                  As parameter of the obsolete render() function, specifies
    ///                  to read all OpenCSG settings from the settings set with
    ///                  setOption() and not from the parameter list of render(),
    ///                  i.e., rendering is done as in the render() function taking
    ///                  only one parameter. 
    enum Algorithm {
        Automatic        = 0,
        Goldfeather      = 1,
        SCS              = 2,
        AlgorithmUnused  = 3
    };

    /// The DepthComplexityAlgorithm specifies the strategy for profiting
    /// from depth complexity when performing the CSG rendering.
    ///   - NoDepthComplexitySampling: Does not employ the depth complexity.
    ///                  This essentially makes the algorithm O(n�), but with
    ///                  low constant costs.
    ///   - DepthComplexitySampling: Calculates the depth complexity k using
    ///                  the stencil buffer. This makes algorithm O(n*k), but
    ///                  with high constant costs. In case of the Goldfeather,
    ///                  the literature denotes this as layered Goldfeather 
    ///                  algorithm.
    ///   - OcclusionQuery: Uses occlusion queries to profit implicitly from
    ///                  depth complexity without calculating it. This is 
    ///                  especially useful for the SCS algorithm where this 
    ///                  strategy is applied at shape level, resulting in a 
    ///                  O(n*k') algorithm (where k' <= k), without significant
    ///                  constant overhead. This strategy requires hardware 
    ///                  occlusion queries (i.E., the OpenGL-extension 
    ///                  GL_ARB_occlusion_query or GL_NV_occlusion_query)
    ///   - DepthComplexityAlgorithmUnused: Invalid input. 
    enum DepthComplexityAlgorithm {
        NoDepthComplexitySampling      = 0,
        OcclusionQuery                 = 1,
        DepthComplexitySampling        = 2,
        DepthComplexityAlgorithmUnused = 3
    };

    /// The OffscreenType sets the type of offscreen buffer which is used for
    /// the internal calculations. 
    ///   - AutomaticOffscreenType: Chooses internally depending on available
    ///                  OpenGL extensions. If graphics hardware supports different
    ///                  extensions, with most precedence ARB frame buffer objects
    ///                  are used, then EXT frame buffer objects.
    ///   - FrameBufferObject: Same as AutomaticOffscreenType.
    ///   - PBuffer (2): Not supported anymore
    ///   - FrameBufferObjectARB: Forces ARB frame buffer objects to be used.
    ///   - FrameBufferObjectEXT: Forces EXT frame buffer objects to be used.
    ///   - OffscreenTypeUnused: Invalid input.
    enum OffscreenType {
        AutomaticOffscreenType = 0,
        FrameBufferObject      = 1,
        FrameBufferObjectARB   = 3,
        FrameBufferObjectEXT   = 4,
        OffscreenTypeUnused    = 5
    };

    /// The Optimization flags set whether a specific kind of rendering per-
    /// formance optimization is enabled or not. This can be set for the
    /// following kind of optimizations:

    ///   - DepthBoundsOptimization: Improves rendering performance by using 
    ///     the depth bounds check found on some graphics hardware. By default,
    ///     this optimization is turned off! When you turn it on, you must
    ///     provide correct bounding boxes for all primitives, in particular
    ///     along the z-axis.

    ///   - CameraOutsideOptimization: This setting enables rendering
    ///     optimizations that are only valid if the camera is known to be
    ///     outside of the CSG model. Currently this applies to the Goldfeather
    ///     algorithm only. By default, this optimization is disabled.
    ///     If your camera is outside of the CSG model, you may enable the
    ///     z-pass setting to check if it works better for you.

    ///     For the Goldfeather algorithm, the setting controls how the parity
    ///     is calculated. The parity value is the number of surfaces in front
    ///     of the surface for that visibility is about to be determined. The
    ///     textbook approach to determine it is to render all surfaces that
    ///     are in front (z-pass). This has the drawback, though, that if the
    ///     camera is inside of the CSG model, some surfaces could be clipped by
    ///     the view frustum, resulting that rendering is not correct. The
    ///     alternative, default approach is to render surfaces behind (z-fail).
    ///     With that, clipping of the surfaces is much less likely to happen
    ///     and easy to avoid. So this is the more robust setting.
    ///     The performance of the two approaches is usually similar.

    /// Each optimization can be independently set
    ///   - OptimizationDefault     to its default value
    ///   - OptimizationForceOn     on (does not check OpenGL extensions)
    ///   - OptimizationOn          on if required OpenGL extensions are supported,
    ///   - OptimizationOff         off
    ///   - OptimizationUnused:     Invalid input.
    enum Optimization {
        OptimizationDefault   = 0,
        OptimizationForceOn   = 1,
        OptimizationOn        = 2,
        OptimizationOff       = 3,
        OptimizationUnused    = 4
    };

    /// Setting the context is required for applications rendering with
    /// OpenCSG in different OpenGL windows with OpenGL contexts that
    /// are not shared. This is needed for internal OpenGL resources,
    /// such as frame buffer objects, which are created in
    /// in the render() method and recycled when render() is called again.
    /// The resources are only valid in the OpenGL context they have
    /// been created (and in contexts shared with this context).
    /// To manage this, this function allows to set the identifier of
    /// resources, which is respected during the render() function.
    /// OpenGL resources are created/reused by the render() function
    /// per context.
    /// The value of context has no specific meaning for OpenCSG;
    /// it is only used as key in a dictionary to access the OpenGL
    /// resources. The default context is 0.
    void setContext(int context);
    /// Returns the current context.
    int getContext();
    /// Releases the OpenGL resources allocated by OpenCSG for the current
    /// context. 
    void freeResources();

} // namespace OpenCSG

#endif // __OpenCSG__opencsg_h__
