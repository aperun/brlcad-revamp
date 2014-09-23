/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include <osgViewer/config/SingleScreen>
#include <osgViewer/Renderer>
#include <osgViewer/View>
#include <osgViewer/GraphicsWindow>

#include <osgViewer/config/SingleWindow>

#include <osg/io_utils>

using namespace osgViewer;

void SingleScreen::configure(osgViewer::View& view) const
{
    osg::ref_ptr<osgViewer::SingleWindow> singleWindow = new SingleWindow(0,0,-1,-1,_screenNum);
    singleWindow->setWindowDecoration(false);
    singleWindow->configure(view);
}
