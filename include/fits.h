#ifndef _FITS_H_
#define _FITS_H_

#include <QFileDevice>
#include <QSize>
#include <QVariant>

#include <abstractfitsstorage.h>
#include <exception.h>

#include <map>
#include <memory>

class FITS {
public:
	class Exception: public ::Exception {
	public:
		explicit Exception(const QString& what);

		virtual void raise() const override;
		virtual QException* clone() const override;
	};

	class UnexpectedEnd: public Exception {
	public:
		UnexpectedEnd();

		virtual void raise() const override;
		virtual QException* clone() const override;
	};

	class WrongHeaderValue: public Exception {
	public:
		WrongHeaderValue(const QString& key, const QString& value);

		virtual void raise() const override;
		virtual QException* clone() const override;
	};

	class UnsupportedBitpix: public WrongHeaderValue {
	public:
		UnsupportedBitpix(const QString& bitpix);

		virtual void raise() const override;
		virtual QException* clone() const override;
	};

	class HeaderUnit {
	private:
		std::map<QString, QString> headers_;
	public:
		HeaderUnit(AbstractFITSStorage::Page& begin, const AbstractFITSStorage::Page& end);

		inline const QString& header(const QString& key) const {
			return headers_.at(key);
		}
		inline const QString& header(const QString& key, const QString& def) const {
			auto it = headers_.find(key);
			return (it != headers_.end() ? it->second : def);
		}
		template<class T> inline T header_as(const QString& key) const {
			QVariant v(headers_.at(key));
			if (!v.canConvert<T>())
				throw WrongHeaderValue(key, v.value<QString>());
			return v.value<T>();
		}
		template<class T> inline T header_as(const QString& key, const T& def) const {
			auto it = headers_.find(key);
			return (it != headers_.end() && QVariant(it->second).canConvert<T>() ?
				qvariant_cast<T>(it->second) : def);
		}

		inline double bscale() const { return header_as<double>("BSCALE", 1.0); }
		inline double bzero()  const { return header_as<double>("BZERO", 0.0); }
	};

	class AbstractDataUnit;
	class ImageDataUnit;
	class EmptyDataUnit;
	template<class T> class DataUnit;

	class AbstractDataUnit {
	private:
		const quint8* data_;
		quint64 length_;
	protected:
		struct VisitorBase {
			virtual ~VisitorBase() = 0;
			virtual void visit(const EmptyDataUnit&) = 0;
			virtual void visit(const DataUnit<quint8>&) = 0;
			virtual void visit(const DataUnit<qint16>&) = 0;
			virtual void visit(const DataUnit<qint32>&) = 0;
			virtual void visit(const DataUnit<qint64>&) = 0;
			virtual void visit(const DataUnit<float>&) = 0;
			virtual void visit(const DataUnit<double>&) = 0;
		};

		virtual void do_apply(VisitorBase* visitor) const = 0;
	public:
		AbstractDataUnit(AbstractFITSStorage::Page& begin, const AbstractFITSStorage::Page& end, quint64 length);
		virtual ~AbstractDataUnit() = 0;

		inline const quint8* data() const { return data_; }
		inline quint64 length() const { return length_; }

		template<class F> void apply(F fun) const {
			struct Visitor: public VisitorBase {
				F* fun_;

				inline Visitor(F* fun): VisitorBase(), fun_(fun) {}

				virtual void visit(const EmptyDataUnit& x) override { (*fun_)(x); };
				virtual void visit(const DataUnit<quint8>& x) override { (*fun_)(x); };
				virtual void visit(const DataUnit<qint16>& x) override { (*fun_)(x); };
				virtual void visit(const DataUnit<qint32>& x) override { (*fun_)(x); };
				virtual void visit(const DataUnit<qint64>& x) override { (*fun_)(x); };
				virtual void visit(const DataUnit<float>& x)  override { (*fun_)(x); };
				virtual void visit(const DataUnit<double>& x) override { (*fun_)(x); };
			};

			Visitor v{&fun};
			do_apply(&v);
		}

		template<class F> static void bitpixToType(const QString& bitpix, F fun);
		static AbstractDataUnit* createFromBitpix(const QString& bitpix, AbstractFITSStorage::Page& begin, const AbstractFITSStorage::Page& end, quint64 height, quint64 width);

		inline       ImageDataUnit* imageDataUnit()       { return dynamic_cast<ImageDataUnit*>(this); }
		inline const ImageDataUnit* imageDataUnit() const { return dynamic_cast<const ImageDataUnit*>(this); }
	};

	class ImageDataUnit: public AbstractDataUnit {
	private:
		quint64 height_;
		quint64 width_;
	public:
		ImageDataUnit(AbstractFITSStorage::Page& begin, const AbstractFITSStorage::Page& end, quint32 element_size, quint64 height, quint64 width);
		virtual ~ImageDataUnit() = 0;

		inline quint64 height() const { return height_; }
		inline quint64 width()  const { return width_; }
		inline QSize size() const { return QSize(width_, height_); }
	};

	class EmptyDataUnit: public AbstractDataUnit {
	protected:
		virtual void do_apply(VisitorBase* visitor) const override {
			visitor->visit(*this);
		}
	public:
		EmptyDataUnit(AbstractFITSStorage::Page& begin, const AbstractFITSStorage::Page& end);
		virtual ~EmptyDataUnit();
	};

	template<class T> class DataUnit: public ImageDataUnit {
	protected:
		virtual void do_apply(VisitorBase* visitor) const override {
			visitor->visit(*this);
		}
	public:
		inline DataUnit(AbstractFITSStorage::Page& begin, const AbstractFITSStorage::Page& end, quint64 height, quint64 width):
			ImageDataUnit(begin, end, sizeof(T), height, width) {}
		inline const T* data() const {
			return reinterpret_cast<const T*>(AbstractDataUnit::data());
		}
		inline quint64 length() const { return AbstractDataUnit::length() / sizeof(T); }
	};

	class HeaderDataUnit {
	private:
		std::unique_ptr<HeaderUnit>       header_;
		std::unique_ptr<AbstractDataUnit> data_;
	public:
		HeaderDataUnit(AbstractFITSStorage::Page& begin, const AbstractFITSStorage::Page& end);

		HeaderDataUnit(HeaderDataUnit&&) = default;
		HeaderDataUnit& operator=(HeaderDataUnit&&) = default;

		inline const HeaderUnit&       header() const { return *header_; }
		inline const AbstractDataUnit& data()   const { return *data_; }
	};
private:
	std::unique_ptr<AbstractFITSStorage> fits_storage_;
	HeaderDataUnit primary_hdu_;

	std::list<HeaderDataUnit> extensions_;

	FITS(AbstractFITSStorage* fits_storage, AbstractFITSStorage::Page begin, const AbstractFITSStorage::Page& end);
public:
	typedef std::list<HeaderDataUnit>::const_iterator const_iterator;

	FITS(AbstractFITSStorage* fits_storage);
	FITS(QFileDevice* file_device);

	inline const HeaderDataUnit&   primary_hdu() const { return primary_hdu_; }
	inline const HeaderUnit&       header_unit() const { return primary_hdu_.header(); }
	inline const AbstractDataUnit& data_unit()   const { return primary_hdu_.data(); }

	const_iterator begin() const { return extensions_.begin(); }
	const_iterator end()   const { return extensions_.end(); }
};

template<class F> void FITS::AbstractDataUnit::bitpixToType(const QString& bitpix, F fun) {
	if (bitpix == "8") {
		fun(static_cast<quint8*>(0));
	} else if (bitpix == "16") {
		fun(static_cast<qint16*>(0));
	} else if (bitpix == "32") {
		fun(static_cast<qint32*>(0));
	} else if (bitpix == "64") {
		fun(static_cast<qint64*>(0));
	} else if (bitpix == "-32") {
		fun(static_cast<float*>(0));
	} else if (bitpix == "-64") {
		fun(static_cast<double*>(0));
	} else {
		throw FITS::UnsupportedBitpix(bitpix);
	}
}

#endif // _FITS_H_
