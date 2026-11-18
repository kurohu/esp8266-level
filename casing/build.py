"""构建电子水平仪外壳，验证并导出 STEP / STL。"""

import os
from pathlib import Path

import cadquery as cq

from cad import model


def _check(name, wp):
    shape = wp.val()
    assert shape is not None, f"{name}: Shape 为空"
    assert shape.isValid(), f"{name}: Shape 无效"
    assert shape.Volume() > 0, f"{name}: 体积为 0"
    solids = shape.Solids()
    assert len(solids) == 1, f"{name}: Solid 数量 = {len(solids)}，预期 1"
    bb = shape.BoundingBox()
    print(
        f"[{name}] solid={len(solids)} 体积={shape.Volume():.1f} mm³ "
        f"尺寸={bb.xlen:.2f}×{bb.ylen:.2f}×{bb.zlen:.2f} mm"
    )
    return shape


def main():
    casing, cover = model.build_model()
    plate = model.build_charger_plate()
    usb_cover = model.build_node_usb_cover()
    case_shape = _check("外壳", casing)
    cover_shape = _check("后盖", cover)
    plate_shape = _check("充电板压板", plate)
    usb_shape = _check("USB 补片", usb_cover)

    bb = case_shape.BoundingBox()
    assert all(abs(actual - expected) < 1e-6 for actual, expected in zip(
        (bb.xlen, bb.ylen, bb.zlen), (model.O_L, model.O_H, model.O_D)
    )), "外壳包围盒尺寸异常"
    cover_assembled = model.installed_cover(cover_shape)
    pcb = model.blk(model.CHG_L, model.CHG_W, model.CHG_T,
                    (model.CHG_LEFT + model.CHG_RIGHT) / 2, model.CHG_Y,
                    (model.CHG_FRONT + model.CHG_REAR) / 2)
    for y in (model.CHG_BOTTOM, model.CHG_TOP):
        pcb = pcb.cut(model.cyl(model.CHG_NOTCH_R, model.CHG_T + 0.4,
                                model.CHG_RIGHT - model.CHG_NOTCH_OFFSET,
                                y, model.CHG_FRONT - 0.2))
    node_seat = model.Z_IN + model.NODE_MOUNT_H
    node = model.blk(model.NODE_L, model.NODE_H, model.NODE_PCB_T,
                     model.NODE_X, model.NODE_Y, node_seat + model.NODE_PCB_T / 2).val()
    usb_path = model.blk(model.WALL + model.NODE_LEFT_GAP, 9.5, 4.0,
                         -model.O_L / 2 + (model.WALL + model.NODE_LEFT_GAP) / 2,
                         model.NODE_USB_Y, model.NODE_USB_Z).val()
    assert abs(usb_shape.BoundingBox().xmin - bb.xmin) < 1e-6, "USB 补片外侧未齐平"
    pairs = (("外壳/USB 补片", case_shape, usb_shape),
             ("后盖/USB 补片", cover_assembled, usb_shape),
             ("外壳/NodeMCU", case_shape, node),
             ("USB 补片/NodeMCU", usb_shape, node),
             ("USB 插入包络/外壳", usb_path, case_shape),
             ("外壳/压板", case_shape, plate_shape),
             ("外壳/后盖", case_shape, cover_assembled),
             ("压板/后盖", plate_shape, cover_assembled),
             ("外壳/PCB", case_shape, pcb.val()),
             ("压板/PCB", plate_shape, pcb.val()))
    for name, first, second in pairs:
        overlap = first.intersect(second).Volume()
        assert overlap < 1e-6, f"{name} 干涉体积 {overlap:.6f} mm³"

    os.makedirs("output", exist_ok=True)
    for name, part in (("casing", casing), ("cover", cover),
                       ("charger_pressure_plate", plate), ("node_usb_cover", usb_cover)):
        for suffix in ("step", "stl"):
            path = Path("output") / f"{name}.{suffix}"
            cq.exporters.export(part, str(path))
            assert path.stat().st_size > 0, f"导出为空：{path}"
            print(f"导出成功：{path}")


if __name__ == "__main__":
    main()
