"""电子水平仪外壳参数化模型（CadQuery）— 第五版：矮薄长条。

- 屏幕居中（圆孔 Ø28）
- NodeMCU 横放屏幕左侧（48x25.4，不增高度）
- MPU6050 位于 NodeMCU 上方，充放电模块在屏幕右侧
- 电池盒放屏幕后面（z 后，Y 与屏幕重叠，不增高度）
- 排针朝盒背面(+Z)，排针空间按实际 8.5mm（不再留大）
- 后盖带定位台阶，嵌入齐平

坐标系：X=长度(对称)，Y=高度(底面 Y=0)，Z=深度(前面板 -Z)。
"""

import cadquery as cq
from cadquery import Vector

# ===================== 外壳（矮薄长条） =====================
O_L = 174.0
O_H = 50.0
O_D = 34.0
WALL = 2.5
FRONT_T = 2.0
R_CORNER = 6.0
R_INNER = R_CORNER - WALL   # 3.5：内外圆角同心，壁厚均匀一致

# ===================== 屏幕（居中） =====================
SCREEN_L, SCREEN_H = 62.0, 37.1
SCREEN_HOLE_D = 29.0
SCREEN_CY = O_H / 2 + 0.05
# 横装，排针朝 -X。圆孔对准有效显示区，而非 PCB 中心。
SCREEN_AA_L, SCREEN_AA_H = 40.8, 30.6
SCREEN_AA_FROM_PIN_EDGE = 5.4 + 2.44
SCREEN_PCB_X = SCREEN_L / 2 - SCREEN_AA_FROM_PIN_EDGE - SCREEN_AA_L / 2
SCREEN_PCB_Y = SCREEN_CY + SCREEN_H / 2 - (4.5 + 28.0 / 2)
SCREEN_PCB_T = 1.2
SCREEN_SEAT_H = 4.0 - SCREEN_PCB_T
SCREEN_POST_D, SCREEN_PILOT_D = 6.0, 1.8  # M2 自攻导孔暂定，按实物试锁

# ===================== 元件 =====================
BATT_L, BATT_H, BATT_D = 77.5, 21.2, 21.7
BATT_HOLE_PITCH = 55.0
BATT_X = 22.5  # 合盖后的 X 坐标，避开屏幕排针
BATT_BOTTOM_GAP = 1.0
BATT_Y = WALL + BATT_BOTTOM_GAP + BATT_H / 2
BATT_RECESS_D = 1.0
BATT_RECESS_CLR = 0.25  # 单边装配余量
BATT_WIRE_RELIEF_R = 6.0
BATT_BLIND_SKIN = 0.8
BUZZER_SEAT_H = 3.0
BUZZER_POST_D = 7.0
BUZZER_HOLE_X, BUZZER_Z = 14.0, 0.0  # 孔心移到原托台位置
BUZZER_PAD_X = BUZZER_HOLE_X + 14.0  # 排针改到 -X 端
ROCKER_D, ROCKER_Y, ROCKER_Z = 15.0, 12.0, 0.0
NODE_L, NODE_H = 48.25, 25.7
NODE_HOLE_PITCH_X, NODE_HOLE_PITCH_Y = 43.5, 20.6
NODE_POST_D, NODE_POST_FLAT = 6.0, 2.5
NODE_PCB_T = 1.6  # 暂定 PCB 裸板厚度
NODE_STACK_H, NODE_FRONT_GAP = 4.5, 0.3
NODE_MOUNT_H = NODE_STACK_H - NODE_PCB_T + NODE_FRONT_GAP
MPU_L, MPU_W = 21.6, 15.9
NODE_LEFT_GAP, NODE_BOTTOM_GAP = 2.0, 1.0
MPU_TOP_GAP = 1.0
MPU_MOUNT_H = 3.0
MPU_POST_D = 6.0
MPU_HOLE_PITCH, MPU_HOLE_INSET = 17.4, 2.55  # 暂按照片比例估算
CHG_L, CHG_W, CHG_T = 25.1, 20.2, 1.3
CHG_TOP_GAP = 2.5
CHG_PORT_TOTAL_H = 4.6
CHG_PORT_OVERHANG = 1.0
CHG_PORT_W, CHG_PORT_H = 9.0, CHG_PORT_TOTAL_H - CHG_T + 0.2  # 暂定侧壁窗口
CHG_PORT_LAND = 1.0
CHG_PORT_R = 0.4
CHG_PORT_CHAMFER = WALL - CHG_PORT_LAND
CHG_NOTCH_OFFSET, CHG_NOTCH_R = 13.0, 1.1  # 图示值，待实测
CHG_LOCATOR_R, CHG_CORNER_SEAT = 0.9, 1.3
CHG_LED_WINDOW_SKIN = 0.4
CHG_LED_WINDOW_L, CHG_LED_WINDOW_H = 9.0, 2.0
CHG_LED_FROM_PORT, CHG_LED_FROM_BOTTOM = 19.25, 1.15  # 灯排中心按照片估算
CHG_WIRE_EXIT_H = 5.0
CHG_PLATE_NOTCH_GAP = 0.3
CHG_TAIL_PAD_W, CHG_TAIL_PAD_D, CHG_TAIL_PAD_PITCH = 4.0, 3.0, 6.0
CHG_TAIL_PAD_CLEAR = 0.4
CHG_PLATE_T, CHG_SCREW_CLEAR = 2.5, 2.9  # 压板厚度与 M2.6 间隙孔
CHG_M3_EAR_R = 3.4

# ===================== 后盖 =====================
LID_T = 2.0        # 盖板厚
STEP_T = 2.5       # 定位台阶厚
LID_CLR = 0.3
LID_M2_D = 2.2

# 派生
IN_L = O_L - 2 * WALL
IN_H = O_H - 2 * WALL
IN_D = O_D - FRONT_T
_CY = O_H / 2
Z_IN = -O_D / 2 + FRONT_T   # 前壁内表面 z
M2_D = 2.2
M26_PILOT_D = 2.3  # 充电板连框固定座和后盖电池盒
M26_POST_PILOT_D = M26_PILOT_D  # 独立柱使用相同的 M2.6 自攻导孔
FRONT_BLIND_SKIN = 0.8
ROOF_BLIND_SKIN = 0.8
SELF_TAP_LEAD = 0.25
NODE_X = -IN_L / 2 + NODE_LEFT_GAP + NODE_L / 2
NODE_Y = WALL + NODE_BOTTOM_GAP + NODE_H / 2
MPU_X = NODE_X - 1.0 + MPU_L / 2  # 相对原布局右移半个 PCB 长度
MPU_Y = O_H - WALL - MPU_TOP_GAP - MPU_W / 2
CHG_SIDE_GAP = 0.175
CHG_VERTICAL_GAP = 0.15
CHG_RIGHT = IN_L / 2 - CHG_SIDE_GAP
CHG_LEFT = CHG_RIGHT - CHG_L
CHG_TOP = O_H - WALL - CHG_TOP_GAP
CHG_BOTTOM = CHG_TOP - CHG_W
CHG_Y = (CHG_TOP + CHG_BOTTOM) / 2
CHG_FRONT = Z_IN + CHG_PORT_TOTAL_H - CHG_T
CHG_REAR = CHG_FRONT + CHG_T
CHG_M3_X = CHG_LEFT - 2.2

# 双按键板长边横放，在外壳上部上下排列。
KEY_L, KEY_W, KEY_T = 17.9, 10.3, 1.6
KEY_HOLE_PITCH = 12.1
KEY_SWITCH_W, KEY_SWITCH_H = 6.0, 5.05  # 本体平面高度含 PCB
KEY_BUMP_H = 5.65  # 四角凸点总高含 PCB
KEY_BUMP_PITCH, KEY_BUMP_POCKET_D = 4.3, 1.65  # 按正面照片估算
KEY_CAP_D, KEY_CAP_H = 3.45, 6.6       # 高度含 PCB
KEY_POST_H = 3.85
KEY_POST_D = 6.0
KEY_POST_SWITCH_CLR = 0.30
KEY_RELIEF_FROM_FRONT = 1.85
KEY_SWITCH_GAP = 0.1  # 原内侧让位净空，孔边减薄量由外侧斜面提供
KEY_WELL_SKIN = 0.4  # 键帽孔边到内侧让位面的剩余壁厚，试打值
KEY_WELL_D, KEY_CLEAR_D = 8.2, 4.2
KEY_X = (SCREEN_PCB_X + SCREEN_L / 2 + CHG_M3_X - CHG_M3_EAR_R) / 2
KEY_TOP_GAP, KEY_ROW_GAP = 1.0, 1.7
KEY_TOP_Y = O_H - WALL - KEY_TOP_GAP - KEY_W / 2
KEY_Y = (KEY_TOP_Y - KEY_W - KEY_ROW_GAP, KEY_TOP_Y)
KEY_BACK_Z = Z_IN + KEY_POST_H + KEY_T


def _button_mount(body):
    front = -O_D / 2
    seat = KEY_BACK_Z - KEY_T
    relief_floor = front + KEY_RELIEF_FROM_FRONT
    relief_depth = Z_IN - relief_floor + 0.2
    well_depth = relief_floor - front - KEY_WELL_SKIN
    bump_floor = KEY_BACK_Z - KEY_BUMP_H - KEY_SWITCH_GAP
    bump_depth = relief_floor - bump_floor
    # 按锥面斜率限制浅窝内缘，使局部剩余厚度不少于孔边壁厚。
    keep_radius = (KEY_CLEAR_D / 2
                   + bump_depth * (KEY_WELL_D - KEY_CLEAR_D) / (2 * well_depth))
    for y in KEY_Y:
        body = body.cut(blk(KEY_SWITCH_W + 0.5, KEY_SWITCH_W + 0.5,
                           relief_depth, KEY_X, y, relief_floor + relief_depth / 2))
        # 外口至通孔连续收窄，孔边保留指定壁厚。
        well = cq.Solid.makeCone(KEY_WELL_D / 2, KEY_CLEAR_D / 2,
                                 well_depth, Vector(KEY_X, y, front))
        body = body.cut(well)
        body = body.cut(cyl(KEY_CLEAR_D / 2, FRONT_T + 0.4,
                            KEY_X, y, front - 0.2))
        keep = cyl(keep_radius, bump_depth + 0.4, KEY_X, y, bump_floor - 0.2)
        for sx in (-1, 1):
            for sy in (-1, 1):
                pocket = cyl(KEY_BUMP_POCKET_D / 2, bump_depth + 0.2,
                             KEY_X + sx * KEY_BUMP_PITCH / 2,
                             y + sy * KEY_BUMP_PITCH / 2, bump_floor)
                body = body.cut(pocket.cut(keep))
        for sign in (-1, 1):
            hole_x = KEY_X + sign * KEY_HOLE_PITCH / 2
            post = _front_post(KEY_POST_D / 2, seat - Z_IN, hole_x, y)
            flat_x = KEY_X + sign * (KEY_SWITCH_W / 2 + KEY_POST_SWITCH_CLR)
            cutter = blk(KEY_POST_D, KEY_POST_D + 1, seat - Z_IN + 0.4,
                         flat_x - sign * KEY_POST_D / 2, y, (Z_IN + seat) / 2)
            body = body.union(post.cut(cutter))
            body = _m26_mount_hole(body, hole_x, y, seat)
    return body


def blk(w, h, d, x, y, z):
    return cq.Workplane("XY").box(w, h, d, centered=(True, True, True)).translate((x, y, z))


def cyl(r, h, x, y, z):
    return cq.Workplane("XY").workplane(offset=z).center(x, y).circle(r).extrude(h)


def _front_post(radius, height, x, y):
    """前壳柱根 R0.2 圆角，柱身直径与支撑高度保持原值。"""
    blend = 0.2
    arc_offset = blend / 2 ** 0.5
    return (cq.Workplane("XZ", origin=(x, y, Z_IN))
            .moveTo(0, 0)
            .lineTo(radius + blend, 0)
            .threePointArc((radius + blend - arc_offset, blend - arc_offset),
                           (radius, blend))
            .lineTo(radius, height)
            .lineTo(0, height)
            .close().revolve(360, (0, 0), (0, 1)))


def cyl_dir(r, h, x, y, z, dx, dy, dz):
    return cq.Solid.makeCylinder(r, h, Vector(x, y, z), Vector(dx, dy, dz))


def _self_tap_hole(body, x, y, bottom, seat, diameter):
    """盲导孔和 45° 入口倒角，底部深度不变。"""
    radius = diameter / 2
    bore = cyl(radius, seat - bottom + 0.2, x, y, bottom)
    lead = cq.Solid.makeCone(radius, radius + SELF_TAP_LEAD, SELF_TAP_LEAD,
                             Vector(x, y, seat - SELF_TAP_LEAD))
    return body.cut(bore).cut(lead)


def _m26_mount_hole(body, x, y, seat, diameter=M26_POST_PILOT_D):
    bottom = -O_D / 2 + FRONT_BLIND_SKIN
    return _self_tap_hole(body, x, y, bottom, seat, diameter)


def _charger_mount(body):
    """前侧托住 PCB，后侧独立压板锁在左端连框螺柱上。"""
    base_z = Z_IN - 0.2
    seat_h = CHG_FRONT - base_z
    notch_x = CHG_RIGHT - CHG_NOTCH_OFFSET
    stile_w, rail_w = 1.5, 1.15
    rail_offset = rail_w / 2 + CHG_VERTICAL_GAP
    frame_x = CHG_LEFT - stile_w / 2 - CHG_SIDE_GAP
    frame_end = IN_L / 2 + 0.3
    rail_h = CHG_REAR - base_z
    frame_left = frame_x - stile_w / 2
    frame_z = base_z + rail_h / 2
    frame = blk(stile_w, CHG_W + 2 * rail_offset + rail_w, rail_h,
                frame_x, CHG_Y, frame_z)
    for edge, inward in ((CHG_BOTTOM, 1), (CHG_TOP, -1)):
        frame = frame.union(blk(frame_end - frame_left, rail_w, rail_h,
                                (frame_end + frame_left) / 2,
                                edge - inward * rail_offset, frame_z))
    # 合并后倒圆，两段外直边共用同一条相切圆弧。
    frame = frame.edges("|Z and <X").fillet(0.4)
    wire_top = CHG_Y - CHG_M3_EAR_R - 0.3
    frame = frame.cut(blk(stile_w + 0.4, CHG_WIRE_EXIT_H, rail_h + 0.4,
                          frame_x, wire_top - CHG_WIRE_EXIT_H / 2, frame_z))
    for edge, inward in ((CHG_BOTTOM, 1), (CHG_TOP, -1)):
        # 定位凸耳只伸进缺口，凸耳底部的微台阶承托缺口边缘。
        frame = frame.union(cyl(CHG_LOCATOR_R, rail_h, notch_x, edge, base_z))
        frame = frame.union(cyl(CHG_NOTCH_R + 0.15, seat_h, notch_x, edge, base_z))
        corner = (cq.Workplane("XY").workplane(offset=base_z)
                  .polyline([(CHG_LEFT - 0.5, edge),
                             (CHG_LEFT + CHG_CORNER_SEAT, edge),
                             (CHG_LEFT - 0.5, edge + inward * CHG_CORNER_SEAT)])
                  .close().extrude(seat_h))
        frame = frame.union(corner)

    # 螺柱延伸至前壳，盲孔从背侧加工。
    boss_base = base_z
    boss_h = CHG_REAR - boss_base
    bridge = blk(frame_x - CHG_M3_X + 0.6, 4.0, boss_h,
                 (frame_x + CHG_M3_X) / 2, CHG_Y, boss_base + boss_h / 2)
    r = CHG_M3_EAR_R
    shoulder_x = CHG_M3_X + 2.0
    boss = (cq.Workplane("XY").workplane(offset=boss_base)
            .moveTo(shoulder_x, CHG_Y - r)
            .lineTo(CHG_M3_X, CHG_Y - r)
            .threePointArc((CHG_M3_X - r, CHG_Y), (CHG_M3_X, CHG_Y + r))
            .lineTo(shoulder_x, CHG_Y + r)
            .close().extrude(boss_h))
    frame = frame.union(bridge).union(boss)
    opening = blk(WALL + 2, CHG_PORT_W, CHG_PORT_H,
                  O_L / 2 - WALL / 2, CHG_Y, Z_IN + CHG_PORT_H / 2
                  ).edges("|X").fillet(CHG_PORT_R)
    # 分别取两个圆角矩形截面，实体放样不依赖工作平面栈。
    sections = []
    for inset in (0.0, CHG_PORT_CHAMFER):
        profile = blk(0.2, CHG_PORT_W + 2 * inset, CHG_PORT_H + 2 * inset,
                      O_L / 2 - CHG_PORT_CHAMFER + inset - 0.1,
                      CHG_Y, Z_IN + CHG_PORT_H / 2)
        sections.append(profile.edges("|X").fillet(CHG_PORT_R + inset)
                        .faces(">X").val().outerWire())
    lead_in = cq.Solid.makeLoft(sections, ruled=True)
    corner_relief = blk(2.0, 2.0, CHG_T + 0.4,
                        CHG_RIGHT - 0.8, CHG_TOP - 0.8,
                        (CHG_FRONT + CHG_REAR) / 2)
    body = body.union(frame).cut(opening).cut(lead_in).cut(corner_relief)
    # 从内面切盲槽，外表面保留同材质透光薄壁。
    led_floor = -O_D / 2 + CHG_LED_WINDOW_SKIN
    led_depth = Z_IN - led_floor + 0.1
    led_window = blk(CHG_LED_WINDOW_L, CHG_LED_WINDOW_H, led_depth,
                     CHG_RIGHT - CHG_LED_FROM_PORT, CHG_BOTTOM + CHG_LED_FROM_BOTTOM,
                     led_floor + led_depth / 2).edges("|Z").fillet(0.5)
    body = body.cut(led_window)
    return _m26_mount_hole(body, CHG_M3_X, CHG_Y, CHG_REAR, M26_PILOT_D)


def build_charger_plate():
    """独立压板，返回装配坐标下的单实体。"""
    left = CHG_LEFT - 1.0
    right = CHG_RIGHT - CHG_NOTCH_OFFSET - CHG_LOCATOR_R - CHG_PLATE_NOTCH_GAP
    bottom, top = CHG_BOTTOM - 1.0, CHG_TOP + 1.0
    r = CHG_M3_EAR_R
    plate = (cq.Workplane("XY").workplane(offset=CHG_REAR)
             .moveTo(right, bottom)
             .lineTo(left, bottom)
             .lineTo(CHG_M3_X, CHG_Y - r)
             .threePointArc((CHG_M3_X - r, CHG_Y), (CHG_M3_X, CHG_Y + r))
             .lineTo(left, top)
             .lineTo(right, top)
             .close().extrude(CHG_PLATE_T))
    # 两个独立通槽避开焊盘，同时保留螺丝耳周围的连续材料。
    slot_left = CHG_LEFT - CHG_TAIL_PAD_CLEAR
    slot_right = CHG_LEFT + CHG_TAIL_PAD_D + CHG_TAIL_PAD_CLEAR
    for side in (-1, 1):
        slot = blk(slot_right - slot_left, CHG_TAIL_PAD_W + 2 * CHG_TAIL_PAD_CLEAR,
                   CHG_PLATE_T + 0.4, (slot_left + slot_right) / 2,
                   CHG_Y + side * CHG_TAIL_PAD_PITCH / 2,
                   CHG_REAR + CHG_PLATE_T / 2).edges("|Z").fillet(0.5)
        plate = plate.cut(slot)
    return plate.cut(cyl(CHG_SCREW_CLEAR / 2, CHG_PLATE_T + 0.4,
                         CHG_M3_X, CHG_Y, CHG_REAR - 0.2))


COVER_SCREW_X = IN_L / 2 - 3.0
COVER_CLEAR_D, COVER_HEAD_D = 3.3, 6.0
COVER_EAR_R, COVER_EAR_H = 3.5, 5.5
COVER_EAR_BLIND_SKIN = 0.8


def _cover_ears(body):
    # 耳座顶面停在后盖台阶前，背侧盲孔保留孔底余厚。
    ear_top = O_D / 2 - (STEP_T - 0.5) - 0.15
    ear_base = ear_top - COVER_EAR_H
    wall_overlap = 0.6
    reach = IN_L / 2 + wall_overlap - COVER_SCREW_X
    for side in (-1, 1):
        x = side * COVER_SCREW_X
        lug = cyl(COVER_EAR_R, COVER_EAR_H, x, _CY, ear_base)
        root = blk(reach, 2 * COVER_EAR_R, COVER_EAR_H,
                   side * (COVER_SCREW_X + reach / 2), _CY, ear_base + COVER_EAR_H / 2)
        # 前面板朝下打印时，斜面每升高 1 mm 向内伸出 1 mm。
        wall_x = IN_L / 2
        tip_x = COVER_SCREW_X - COVER_EAR_R
        radius = 0.5
        arc_x = tip_x + radius
        arc_z = ear_base + radius * (2 ** 0.5 - 1)
        diagonal = radius / 2 ** 0.5
        # 圆弧分别与 45° 斜面和耳座竖边相切。
        gusset = (cq.Workplane("XZ", origin=(0, _CY + COVER_EAR_R, 0))
                  .moveTo(side * (wall_x + wall_overlap), ear_base - (wall_x - tip_x))
                  .lineTo(side * wall_x, ear_base - (wall_x - tip_x))
                  .lineTo(side * (arc_x - diagonal), arc_z - diagonal)
                  .threePointArc((side * (arc_x - radius * 0.9238795325),
                                  arc_z - radius * 0.3826834324), (side * tip_x, arc_z))
                  .lineTo(side * tip_x, ear_base + 0.3)
                  .lineTo(side * (wall_x + wall_overlap), ear_base + 0.3)
                  .close().extrude(2 * COVER_EAR_R))
        clip_bottom = ear_base - (wall_x - tip_x) - 0.1
        clip_h = ear_base + 0.4 - clip_bottom
        outline = cyl(COVER_EAR_R, clip_h, x, _CY, clip_bottom)
        outline = outline.union(blk(reach, 2 * COVER_EAR_R, clip_h,
                                   side * (COVER_SCREW_X + reach / 2), _CY,
                                   clip_bottom + clip_h / 2))
        gusset = gusset.intersect(outline)
        body = body.union(lug.union(root).union(gusset))
        body = _self_tap_hole(body, x, _CY, ear_base + COVER_EAR_BLIND_SKIN, ear_top, 2.5)
    return body


def _buzzer_print_ramp(x, width, front_z, roof, seat):
    # 前面板朝下时沿 +Z 生长，先从顶内壁起坡，再搭接原支撑。
    start_z = front_z - (roof - seat)
    return (cq.Workplane("YZ", origin=(x - width / 2, 0, 0))
            .polyline([(roof + 0.2, start_z), (roof, start_z),
                       (seat, front_z), (seat, BUZZER_Z),
                       (roof + 0.2, BUZZER_Z)])
            .close().extrude(width))


def _roof_buzzer_and_rocker(body):
    roof = O_H - WALL
    seat = roof - BUZZER_SEAT_H
    boss = cyl_dir(BUZZER_POST_D / 2, BUZZER_SEAT_H + 0.2, BUZZER_HOLE_X, seat,
                   BUZZER_Z, 0, 1, 0)
    pad = blk(4.0, BUZZER_SEAT_H + 0.2, 9.0, BUZZER_PAD_X,
              (roof + seat + 0.2) / 2, BUZZER_Z).edges("|Y").fillet(0.6)
    boss = boss.fuse(_buzzer_print_ramp(BUZZER_HOLE_X, BUZZER_POST_D, BUZZER_Z - BUZZER_POST_D / 2,
                                       roof, seat).val())
    pad = pad.union(_buzzer_print_ramp(BUZZER_PAD_X, 4.0, BUZZER_Z - 4.5,
                                      roof, seat))
    hole = cyl_dir(M26_POST_PILOT_D / 2, O_H - ROOF_BLIND_SKIN - seat + 0.2,
                   BUZZER_HOLE_X, seat - 0.2, BUZZER_Z, 0, 1, 0)
    lead = cq.Solid.makeCone(M26_POST_PILOT_D / 2 + SELF_TAP_LEAD,
                             M26_POST_PILOT_D / 2, SELF_TAP_LEAD,
                             Vector(BUZZER_HOLE_X, seat, BUZZER_Z), Vector(0, 1, 0))
    opening = cyl_dir(ROCKER_D / 2, WALL + 0.4, O_L / 2 + 0.2,
                      ROCKER_Y, ROCKER_Z, -1, 0, 0)
    # 卡扣包络在右下内圆角处局部让位，保留外侧安装孔尺寸。
    clip_relief = cyl_dir(8.7, 4.0, IN_L / 2, ROCKER_Y, ROCKER_Z, -1, 0, 0)
    return body.union(boss).union(pad).cut(hole).cut(lead).cut(opening).cut(clip_relief)


BOTTOM_TIE_X = ((NODE_X + NODE_L / 2) + (SCREEN_PCB_X - SCREEN_L / 2)) / 2
WIRE_TIE_POINTS = ((BOTTOM_TIE_X, False), (-33.0, True), (38.0, True))
WIRE_TIE_Z = 4.0


def _wire_tie_ears(body):
    """沿 Z 穿扎带，横跨 X 向线束；前侧斜根按 +Z 方向打印。"""
    for x, on_roof in WIRE_TIE_POINTS:
        wall_y = O_H - WALL if on_roof else WALL
        inward = -1 if on_roof else 1
        y = lambda offset: wall_y + inward * offset
        plane = cq.Workplane("YZ", origin=(x - 3.0, 0, WIRE_TIE_Z))
        ear = (plane.polyline([(y(-0.3), -2.8), (y(0), -2.8),
                               (y(4.6), 1.8), (y(4.6), 4.6), (y(-0.3), 4.6)])
               .close().extrude(6.0))
        slot = blk(3.4, 2.2, 8.0, x, y(2.3), WIRE_TIE_Z + 0.9
                   ).edges("|Z").fillet(0.35)
        body = body.union(ear.cut(slot))
    return body


# USB 母座及螺丝头包络暂定，按实际烧录线和螺丝试装。
NODE_USB_Y = NODE_Y
NODE_USB_PIN_FACE_OFFSET = 1.4  # 母座中心距元件面暂定值
NODE_USB_Z = Z_IN + NODE_MOUNT_H + NODE_PCB_T + NODE_USB_PIN_FACE_OFFSET
NODE_USB_OPEN_Y, NODE_USB_OPEN_Z = 11.9, 5.4
NODE_USB_SCREW_Z = NODE_USB_Z + 8.0
NODE_USB_LIP, NODE_USB_CLR = 1.2, 0.25
NODE_USB_HEAD_D, NODE_USB_HEAD_H = 4.3, 1.5  # 平底沉孔暂定尺寸
NODE_USB_SCREW_CLEAR = 2.3
NODE_USB_PAD_T, NODE_USB_PILOT_DEPTH = 2.1, 1.5
NODE_USB_BLIND_SKIN = 0.8


def _node_usb_profile(depth, height, width, x, y, radius):
    return blk(depth, height, width, x, y, NODE_USB_Z).edges("|X").fillet(radius)


def _node_usb_flange(depth, x, clearance):
    front = NODE_USB_Z - NODE_USB_OPEN_Z / 2 - 0.9
    rear = NODE_USB_SCREW_Z + 3.65
    return blk(depth, NODE_USB_OPEN_Y + 3.0 + 2 * clearance,
               rear - front + 2 * clearance, x, NODE_USB_Y, (front + rear) / 2
               ).edges("|X").fillet(1.0 + clearance)


def _node_usb_mount(body):
    outside = -O_L / 2
    inward = (1, 0, 0)
    y, z = NODE_USB_Y, NODE_USB_SCREW_Z
    seat = outside + NODE_USB_PAD_T
    boss = cyl_dir(3.5, NODE_USB_PAD_T + NODE_USB_PILOT_DEPTH + NODE_USB_BLIND_SKIN - WALL + 0.2,
                   outside + WALL - 0.2, y, z, *inward)
    body = body.union(boss)
    body = body.cut(_node_usb_flange(NODE_USB_LIP + 0.2,
                                     outside + (NODE_USB_LIP - 0.2) / 2, 0))
    # 扩孔后靠前的一边进入前面板内表面，沿插入路径让位至 PCB 板缘。
    path_depth = WALL + NODE_LEFT_GAP
    body = body.cut(_node_usb_profile(path_depth + 0.2, NODE_USB_OPEN_Y, NODE_USB_OPEN_Z,
                                      outside + (path_depth - 0.2) / 2, NODE_USB_Y, 0.7))
    body = body.cut(cyl_dir(3.2, NODE_USB_PAD_T - NODE_USB_LIP + 0.2,
                           outside + NODE_USB_LIP - 0.2, y, z, *inward))
    return body.cut(cyl_dir(SCREEN_PILOT_D / 2, NODE_USB_PILOT_DEPTH + 0.1,
                            seat - 0.1, y, z, *inward))


def build_node_usb_cover():
    outside = -O_L / 2
    clr = NODE_USB_CLR
    tab_depth = WALL - clr
    plate = _node_usb_flange(NODE_USB_LIP, outside + NODE_USB_LIP / 2, -clr)
    tab = _node_usb_profile(tab_depth, NODE_USB_OPEN_Y - 2 * clr,
                            NODE_USB_OPEN_Z - 2 * clr, outside + tab_depth / 2,
                            NODE_USB_Y, 0.7 - clr)
    y, z = NODE_USB_Y, NODE_USB_SCREW_Z
    pad = cyl_dir(3.2 - clr, NODE_USB_PAD_T, outside, y, z, 1, 0, 0)
    plate = plate.union(tab).union(pad)
    plate = plate.cut(cyl_dir(NODE_USB_SCREW_CLEAR / 2, NODE_USB_PAD_T + 0.4,
                             outside - 0.2, y, z, 1, 0, 0))
    return plate.cut(cyl_dir(NODE_USB_HEAD_D / 2, NODE_USB_HEAD_H + 0.2,
                             outside - 0.2, y, z, 1, 0, 0))


def build_casing():
    body = cq.Workplane("XY").box(O_L, O_H, O_D, centered=(True, False, True))
    body = body.edges("|Z").fillet(R_CORNER)
    body = body.cut(blk(IN_L, IN_H, IN_D, 0, _CY, FRONT_T / 2))
    # 内腔四角竖直内棱圆角
    body = body.edges("|Z").fillet(R_INNER)

    # 正面圆孔（屏幕显示窗，居中）+ 外缘倾斜倒角
    body = body.cut(cyl(SCREEN_HOLE_D / 2, FRONT_T + 2, 0, SCREEN_CY, -O_D / 2))
    body = body.faces("<Z").edges("%circle").chamfer(1.2)

    # 屏幕四根 M2 自攻固定柱，保留屏幕贴合高度。
    for x in (SCREEN_PCB_X - 29.0, SCREEN_PCB_X + 29.0):
        for y in (SCREEN_CY - 14.0, SCREEN_CY + 14.0):
            body = body.union(_front_post(SCREEN_POST_D / 2, SCREEN_SEAT_H, x, y))
            bottom = -O_D / 2 + FRONT_BLIND_SKIN
            body = body.cut(cyl(SCREEN_PILOT_D / 2, Z_IN + SCREEN_SEAT_H - bottom + 0.2,
                                x, y, bottom))

    # ---- NodeMCU 4 角 M2 螺柱（横放，屏幕左侧） ----
    node_pts = [(NODE_X + sx * NODE_HOLE_PITCH_X / 2,
                 NODE_Y + sy * NODE_HOLE_PITCH_Y / 2)
                for sx in (1, -1) for sy in (1, -1)]
    for x, y in node_pts:
        inward = 1 if x < NODE_X else -1
        post = _front_post(NODE_POST_D / 2, NODE_MOUNT_H, x, y)
        # 每端的排针焊点都在安装孔朝板中心的一侧。
        flat = blk(NODE_POST_D, NODE_POST_D + 1, NODE_MOUNT_H + 0.4,
                   x + inward * (NODE_POST_FLAT + NODE_POST_D / 2),
                   y, Z_IN + NODE_MOUNT_H / 2)
        body = body.union(post.cut(flat))
        body = _m26_mount_hole(body, x, y, Z_IN + NODE_MOUNT_H)

    # MPU 固定孔朝上，排针朝下。横向平台避开排针焊点带。
    mpu_hole_y = MPU_Y + MPU_W / 2 - MPU_HOLE_INSET
    mpu_rest_y = MPU_Y - MPU_W / 2 + 4.5
    for side in (-1, 1):
        hole_x = MPU_X + side * MPU_HOLE_PITCH / 2
        body = body.union(_front_post(MPU_POST_D / 2, MPU_MOUNT_H, hole_x, mpu_hole_y))
        body = _m26_mount_hole(body, hole_x, mpu_hole_y, Z_IN + MPU_MOUNT_H)
    mpu_rest = blk(MPU_L - 3.0, 3.0, MPU_MOUNT_H,
                   MPU_X, mpu_rest_y, Z_IN + MPU_MOUNT_H / 2)
    body = body.union(mpu_rest.edges("|Z").fillet(0.6))

    # 电池盒放在屏幕后方，固定结构待实物走线确认。

    body = _roof_buzzer_and_rocker(_cover_ears(_button_mount(_charger_mount(body))))
    return _node_usb_mount(_wire_tie_ears(body))


def build_back_cover():
    lid = blk(O_L, O_H, LID_T, 0, _CY, LID_T / 2)
    lid = lid.edges("|Z").fillet(R_CORNER)
    sw = IN_L - 2 * LID_CLR
    sh = IN_H - 2 * LID_CLR
    step = blk(sw, sh, STEP_T, 0, _CY, LID_T + STEP_T / 2 - 0.5)
    step = step.edges("|Z").fillet(R_INNER)
    lid = lid.union(step)
    # 90 度沉头，后盖单件的外表面为 Z=0。
    clear_r, head_r = COVER_CLEAR_D / 2, COVER_HEAD_D / 2
    for side in (-1, 1):
        x = side * COVER_SCREW_X
        lid = lid.cut(cyl(clear_r, LID_T + STEP_T + 0.4, x, _CY, -0.2))
        countersink = cq.Solid.makeCone(head_r, clear_r, head_r - clear_r,
                                        Vector(x, _CY, 0))
        lid = lid.cut(countersink)
    # 电池盒预装在后盖内侧，沿长边两孔对称固定。
    step_top = LID_T + STEP_T - 0.5
    seat = step_top - BATT_RECESS_D
    cut_depth = BATT_RECESS_D + 0.2
    cover_batt_x = -BATT_X  # 后盖绕 Y 轴翻转合盖，X 与 Z 同时反向
    lid = lid.cut(blk(BATT_L + 2 * BATT_RECESS_CLR, BATT_H + 2 * BATT_RECESS_CLR,
                      cut_depth, cover_batt_x, BATT_Y, seat + cut_depth / 2))
    for side in (-1, 1):
        end_x = cover_batt_x + side * (BATT_L / 2 + BATT_RECESS_CLR)
        lid = lid.cut(cyl(BATT_WIRE_RELIEF_R, cut_depth, end_x, BATT_Y, seat))
    for x in (cover_batt_x - BATT_HOLE_PITCH / 2, cover_batt_x + BATT_HOLE_PITCH / 2):
        lid = _self_tap_hole(lid, x, BATT_Y, BATT_BLIND_SKIN, seat, M26_PILOT_D)
    return lid


def installed_cover(shape):
    """后盖保持上下方向，绕竖轴旋转后贴合底壳。"""
    return shape.rotate((0, 0, 0), (0, 1, 0), 180).translate(
        (0, 0, O_D / 2 + LID_T))


def build_model():
    return build_casing(), build_back_cover()


b, l = build_model()
try:
    show_object(b, name="casing")
    show_object(installed_cover(l.val()), name="back_cover")
    show_object(build_charger_plate(), name="charger_pressure_plate")
    show_object(build_node_usb_cover(), name="node_usb_cover")
except NameError:
    pass
