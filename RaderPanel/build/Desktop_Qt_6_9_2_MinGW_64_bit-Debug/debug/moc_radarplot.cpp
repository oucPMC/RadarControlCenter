/****************************************************************************
** Meta object code from reading C++ file 'radarplot.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../radarplot.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'radarplot.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9RadarPlotE_t {};
} // unnamed namespace

template <> constexpr inline auto RadarPlot::qt_create_metaobjectdata<qt_meta_tag_ZN9RadarPlotE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "RadarPlot",
        "startScan",
        "",
        "stopScan",
        "updateTrack",
        "id",
        "angleDeg",
        "distanceRatio",
        "speed",
        "latitude",
        "longitude",
        "type",
        "updateScan"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'startScan'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'stopScan'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateTrack'
        QtMocHelpers::SlotData<void(int, double, double, double, double, double, int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { QMetaType::Double, 6 }, { QMetaType::Double, 7 }, { QMetaType::Double, 8 },
            { QMetaType::Double, 9 }, { QMetaType::Double, 10 }, { QMetaType::Int, 11 },
        }}),
        // Slot 'updateScan'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<RadarPlot, qt_meta_tag_ZN9RadarPlotE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject RadarPlot::staticMetaObject = { {
    QMetaObject::SuperData::link<QwtPolarPlot::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9RadarPlotE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9RadarPlotE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9RadarPlotE_t>.metaTypes,
    nullptr
} };

void RadarPlot::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<RadarPlot *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->startScan(); break;
        case 1: _t->stopScan(); break;
        case 2: _t->updateTrack((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[7]))); break;
        case 3: _t->updateScan(); break;
        default: ;
        }
    }
}

const QMetaObject *RadarPlot::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RadarPlot::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9RadarPlotE_t>.strings))
        return static_cast<void*>(this);
    return QwtPolarPlot::qt_metacast(_clname);
}

int RadarPlot::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtPolarPlot::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
