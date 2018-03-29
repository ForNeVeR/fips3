/*
 *  Copyright (C) 2018  Matwey V. Kornilov <matwey.kornilov@gmail.com>
 *                      Konstantin Malanchev <hombit@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ABSTRACTOPENGLPLAN_H
#define _ABSTRACTOPENGLPLAN_H

#include <QObject>
#include <QString>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>

#include <fits.h>
#include <abstractopengltexture.h>
#include <openglplane.h>
#include <openglshaderprogram.h>

class AbstractOpenGLPlan:
	public QObject {
private:
	QString name_;
	OpenGLPlane plane_;
	OpenGLShaderProgram program_;
public:
	AbstractOpenGLPlan(const QString& name, const FITS::AbstractHeaderDataUnit& hdu, QObject* parent = Q_NULLPTR);
	virtual ~AbstractOpenGLPlan() = 0;

	inline const QString& name() const { return name_; }
	inline const OpenGLPlane& plane() const { return plane_; }
	inline OpenGLShaderProgram& program() { return program_; }

	virtual QString fragmentShaderSourceCode() = 0;
	virtual QString vertexShaderSourceCode();
	virtual AbstractOpenGLTexture& imageTexture() = 0;

	bool initialize();
};

#endif // _ABSTRACTOPENGLPLAN_H
