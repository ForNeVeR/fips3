#include <openglshaderunifroms.h>

OpenGLShaderUniforms::OpenGLShaderUniforms(quint8 channels, quint8 channel_size, double bzero, double bscale) :
		channels(channels),
		channel_size(channel_size),
		bzero(bzero),
		bscale(bscale),
		base(1 << (8 * channel_size)) {
	Q_ASSERT(channels > 0 && channels <= 4);
	if (channel_size > 0) {
		for (quint8 i = 0; i < channels; ++i) {
			const auto j = channels - i - 1;
			a_[j] = static_cast<GLfloat >(std::pow(2, 8 * channel_size * i) * (std::pow(2, 8 * channel_size) - 1));
		}
	} else {
		a_[0] = 1;
		Q_ASSERT(channels == 1);
	}
}

void OpenGLShaderUniforms::setMinMax(const std::pair<double,double>& minmax) {
	if (minmax != minmax_) {
		minmax_ = minmax;
		update_cz();
	}
}

void OpenGLShaderUniforms::setColorMapSize(int colormap_size) {
	if (colormap_size != colormap_size_) {
		colormap_size_ = colormap_size;
		update_cz();
	}
}

void OpenGLShaderUniforms::update_cz() {
	const auto alpha = (1.0 - 1.0 / colormap_size_) / (minmax_.second - minmax_.first);
	auto minus_d = - 0.5 / (alpha * colormap_size_) + minmax_.first - bzero;
	if (channel_size > 0) {
		for (quint8 i = 0; i < channels; ++i) {
			c_[i] = static_cast<GLfloat>(bscale * alpha * a_[i]);
		}
		for (quint8 i = 0; i < channels - 1; ++i) {
			const auto j = channels - i - 1;
			const auto integer_part = std::floor(minus_d / base);
			z_[j] = static_cast<GLfloat>((minus_d / base - integer_part) * base / (base - 1));
			minus_d = integer_part;
		}
		z_[0] = static_cast<GLfloat>(minus_d / (base - 1));
	} else {
		c_[0] = static_cast<GLfloat>(bscale * alpha * a_[0]);
		z_[0] = static_cast<GLfloat>(minus_d / a_[0]);
	}
}
