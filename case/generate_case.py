# Parametric case for the Sandbar Clock. Run from this folder with FreeCADCmd:
#   "C:\Program Files\FreeCAD 1.1\bin\FreeCADCmd.exe" generate_case.py
# Each part is exported in its print orientation, with no supports needed.

import math
import os
import sys
import zlib

import FreeCAD as App
import Mesh
import MeshPart
import numpy as np
import Part
from FreeCAD import Matrix, Vector

OUT = os.path.abspath(os.environ.get("CASE_DIR", os.getcwd()))

# ---- Measure these on your panels and USB-C socket ----------------------------
LED_PITCH = 8.0        # Centre-to-centre LED spacing. A printed grid at 8.0 fits the real panels.
PCB_WIDTH = 65.1       # Width of one panel, measured across the side that meets the other panel.
SOCKET_HOLE_W = 13.8   # Snap-in USB-C socket. This size was the best fit in a printed test piece.
SOCKET_HOLE_H = 7.1    # The socket's flange is 15.5 x 9, so this leaves it overlapping the wall all round.
SOCKET_HOLE_R = 2.5
SOCKET_CENTRE_Z = 6.0

# ---- Design choices -----------------------------------------------------------
COLS, ROWS = 16, 8
TILT_DEG = 12.0
GRID_WALL = 0.8
DIFFUSER_THICKNESS = 0.8
FRONT_THICKNESS = 1.6
GRID_HEIGHT = 10.0     # From the diffuser to the panel's PCB.
GRID_WALL_SHORT = 1.0  # Walls stop this far short of the PCB, to clear the small capacitors.
SHELL_WALL = 3.0
BODY_DEPTH = 25.0      # Front housing, front face to rear rim.
LID_THICKNESS = 2.5
LID_LIP = 2.0
FIT = 0.2              # Clearance between parts that slide together.
POCKET_FIT = 0.25      # Clearance between the screen and the base.
CAVITY_HALF_W = 72.0
CAVITY_HALF_H = 35.0
BOSS = 6.0
SCREW_PILOT_R = 1.25   # M3 self-tapping screws.
SCREW_CLEAR_R = 1.7
PCB_THICKNESS = 1.6
C3_WIDTH = 18.0

# ---- Derived ------------------------------------------------------------------
SEAM = PCB_WIDTH - 8 * LED_PITCH
ARRAY_HALF_W = 8 * LED_PITCH + SEAM / 2
ARRAY_HALF_H = 4 * LED_PITCH
W = 2 * (CAVITY_HALF_W + SHELL_WALL)
H = 2 * (CAVITY_HALF_H + SHELL_WALL)
TOTAL_DEPTH = BODY_DEPTH + LID_THICKNESS
GRID_Z = FRONT_THICKNESS + DIFFUSER_THICKNESS
PCB_FRONT_Z = GRID_Z + GRID_HEIGHT
PCB_BACK_Z = PCB_FRONT_Z + PCB_THICKNESS
SCREW_X = CAVITY_HALF_W - BOSS / 2
SCREW_Y = CAVITY_HALF_H - BOSS / 2
BASE_HALF_W = W / 2 + 6
BASE_REAR = 58.0
BASE_FLOOR_V = 12.0
CHAMBER_FLOOR = 2.0
CHAMBER_TOP = 10.0
CHAMBER_HALF_W = 9.0
REAR_WALL = 2.0        # Sets where the socket's clips grip, so it's matched to SOCKET_HOLE_W.
THETA = math.radians(TILT_DEG)


def box(x0, x1, y0, y1, z0, z1):
    return Part.makeBox(x1 - x0, y1 - y0, z1 - z0, Vector(x0, y0, z0))


def cylinder(x, y, z0, z1, r):
    return Part.makeCylinder(r, z1 - z0, Vector(x, y, z0))


def prism(points, z0, z1):
    wire = Part.makePolygon([Vector(x, y, z0) for x, y in points] + [Vector(points[0][0], points[0][1], z0)])
    return Part.Face(wire).extrude(Vector(0, 0, z1 - z0))


def notched_outline(half_w, half_h, inset=0.0):
    a = half_w - inset
    b = half_h - inset
    c = half_w - BOSS - inset
    d = half_h - BOSS - inset
    return [(c, b), (-c, b), (-c, d), (-a, d), (-a, -d), (-c, -d), (-c, -b), (c, -b), (c, -d), (a, -d), (a, d), (c, d)]


def rounded_slab(z0, z1):
    slab = box(-W / 2, W / 2, -H / 2, H / 2, z0, z1)
    corners = [e for e in slab.Edges if abs(e.Vertexes[0].Z - e.Vertexes[1].Z) > 0.1]
    return slab.makeFillet(8.0, corners)


def chamfer_at(shape, z, size):
    edges = [e for e in shape.Edges if all(abs(v.Z - z) < 1e-4 for v in e.Vertexes)]
    return shape.makeChamfer(size, edges)


def union(shapes):
    result = shapes[0]
    for shape in shapes[1:]:
        result = result.fuse(shape)
    return result.removeSplitter()


def cut_all(shape, cutters):
    return shape.cut(Part.makeCompound(cutters))


def build_diffuser():
    return prism(notched_outline(CAVITY_HALF_W, CAVITY_HALF_H, FIT), 0, DIFFUSER_THICKNESS)


def cell_centres():
    xs = []
    for i in range(COLS // 2):
        offset = SEAM / 2 + LED_PITCH * (i + 0.5)
        xs += [offset, -offset]
    ys = [LED_PITCH * (j - (ROWS - 1) / 2) for j in range(ROWS)]
    return sorted(xs), ys


def build_grid():
    body = prism(notched_outline(CAVITY_HALF_W, CAVITY_HALF_H, FIT), 0, GRID_HEIGHT)
    size = LED_PITCH - GRID_WALL
    xs, ys = cell_centres()
    cutters = [box(x - size / 2, x + size / 2, y - size / 2, y + size / 2, -1, GRID_HEIGHT + 1) for x in xs for y in ys]
    grid = cut_all(body, cutters)
    recess = box(-ARRAY_HALF_W - 0.1, ARRAY_HALF_W + 0.1, -ARRAY_HALF_H - 0.2, ARRAY_HALF_H + 0.2,
                 GRID_HEIGHT - GRID_WALL_SHORT, GRID_HEIGHT + 1)
    grid = grid.cut(recess)
    pillars = []
    near, far = 2 * LED_PITCH + SEAM / 2, 6 * LED_PITCH + SEAM / 2
    for x in (-far, -near, near, far):
        for y in (-2 * LED_PITCH, 2 * LED_PITCH):
            pillars.append(box(x - 0.7, x + 0.7, y - 0.7, y + 0.7, GRID_HEIGHT - GRID_WALL_SHORT - 0.5, GRID_HEIGHT))
    return union([grid] + pillars)


def window_bevel():
    a, b, grow = ARRAY_HALF_W, ARRAY_HALF_H, 0.7
    def wire(u, v, z):
        points = [Vector(-u, -v, z), Vector(u, -v, z), Vector(u, v, z), Vector(-u, v, z)]
        return Part.makePolygon(points + [points[0]])
    return Part.makeLoft([wire(a + grow, b + grow, 0), wire(a, b, grow)], True)


def build_housing():
    shell = chamfer_at(rounded_slab(0, BODY_DEPTH), 0, 1.2)
    cavity = prism(notched_outline(CAVITY_HALF_W, CAVITY_HALF_H), FRONT_THICKNESS, BODY_DEPTH + 1)
    window = box(-ARRAY_HALF_W, ARRAY_HALF_W, -ARRAY_HALF_H, ARRAY_HALF_H, -1, FRONT_THICKNESS + 1)
    wire_hole = box(-8, 8, -H / 2 - 1, -CAVITY_HALF_H + 0.5, PCB_BACK_Z + 0.5, BODY_DEPTH - LID_LIP - 0.5)
    pilots = [cylinder(sx * SCREW_X, sy * SCREW_Y, BODY_DEPTH - 14, BODY_DEPTH + 1, SCREW_PILOT_R)
              for sx in (-1, 1) for sy in (-1, 1)]
    return cut_all(shell, [cavity, window, window_bevel(), wire_hole] + pilots).removeSplitter()


def build_lid():
    plate = chamfer_at(rounded_slab(0, LID_THICKNESS), 0, 0.8)
    lip = prism(notched_outline(CAVITY_HALF_W, CAVITY_HALF_H, FIT), LID_THICKNESS, LID_THICKNESS + LID_LIP)
    lip = lip.cut(prism(notched_outline(CAVITY_HALF_W, CAVITY_HALF_H, FIT + 1.6), LID_THICKNESS - 1, LID_THICKNESS + LID_LIP + 1))
    post_top = TOTAL_DEPTH - PCB_BACK_Z
    posts = [cylinder(x, y, LID_THICKNESS - 0.5, post_top, 2.5) for x in (-48, -16, 16, 48) for y in (-18, 18)]
    half = C3_WIDTH / 2 + 0.4
    cradle = [box(-half - 1.4, -half, -12, 12, LID_THICKNESS - 0.5, LID_THICKNESS + 4),
              box(half, half + 1.4, -12, 12, LID_THICKNESS - 0.5, LID_THICKNESS + 4),
              box(-half - 1.4, half + 1.4, 12, 13.4, LID_THICKNESS - 0.5, LID_THICKNESS + 4)]
    lid = union([plate, lip] + posts + cradle)
    holes, cones = [], []
    for sx in (-1, 1):
        for sy in (-1, 1):
            x, y = sx * SCREW_X, sy * SCREW_Y
            holes.append(cylinder(x, y, -1, LID_THICKNESS + LID_LIP + 1, SCREW_CLEAR_R))
            cones.append(Part.makeCone(3.2, SCREW_CLEAR_R, 1.5, Vector(x, y, 0)))
    return cut_all(lid, holes + cones).removeSplitter()


def body_to_base_matrix():
    # Body axes: x across, y up the screen, z towards the rear. Base axes: X across, Y rearwards, Z up.
    s, c = math.sin(THETA), math.cos(THETA)
    m = Matrix()
    m.A11, m.A12, m.A13, m.A14 = -1, 0, 0, 0
    m.A21, m.A22, m.A23, m.A24 = 0, s, c, (H / 2) * s
    m.A31, m.A32, m.A33, m.A34 = 0, c, -s, BASE_FLOOR_V + (H / 2) * c
    return m


def body_point(z, y):
    # Position in the base's Y/Z plane of a point at depth z and height y above the bottom of the screen.
    s, c = math.sin(THETA), math.cos(THETA)
    return z * c + y * s, BASE_FLOOR_V - z * s + y * c


def build_base():
    knots = [(-16, 1.2), (-10, 4.2), (-3, 13.5), (6, 20.0), (20, 24.0), (34, 23.2), (47, 18.6), (BASE_REAR, 14.0)]
    x0 = -BASE_HALF_W
    curve = Part.BSplineCurve()
    curve.interpolate([Vector(x0, u, v) for u, v in knots])
    a, b = Vector(x0, knots[-1][0], knots[-1][1]), Vector(x0, knots[-1][0], 0)
    c_, d = Vector(x0, knots[0][0], 0), Vector(x0, knots[0][0], knots[0][1])
    wire = Part.Wire([curve.toShape(), Part.makeLine(a, b), Part.makeLine(b, c_), Part.makeLine(c_, d)])
    dune = Part.Face(wire).extrude(Vector(2 * BASE_HALF_W, 0, 0))
    ends = [e for e in dune.Edges if isinstance(e.Curve, Part.BSplineCurve)]
    try:
        rounded = dune.makeFillet(3.0, ends)
        if rounded.isValid():
            dune = rounded
    except Part.OCCError:
        print("Base end fillet skipped")

    f = POCKET_FIT
    corners = [body_point(-f, 0), body_point(TOTAL_DEPTH + f, 0), body_point(TOTAL_DEPTH + f, 90), body_point(-f, 90)]
    wire2 = Part.makePolygon([Vector(-W / 2 - f, u, v) for u, v in corners] + [Vector(-W / 2 - f, corners[0][0], corners[0][1])])
    pocket = Part.Face(wire2).extrude(Vector(W + 2 * f, 0, 0))

    chamber = box(-CHAMBER_HALF_W, CHAMBER_HALF_W, 13, BASE_REAR - REAR_WALL, CHAMBER_FLOOR, CHAMBER_TOP)
    hole = socket_hole(SOCKET_HOLE_W, SOCKET_HOLE_H, REAR_WALL + 2)
    # Turn the hole from pointing up to pointing forwards through the rear wall.
    hole.Placement = App.Placement(Vector(0, BASE_REAR + 1, SOCKET_CENTRE_Z), App.Rotation(Vector(1, 0, 0), 90))
    return cut_all(dune, [pocket, chamber, hole]).removeSplitter()


def socket_hole(width, height, length):
    cutter = box(-width / 2, width / 2, -height / 2, height / 2, 0, length)
    edges = [e for e in cutter.Edges if abs(e.Vertexes[0].Z - e.Vertexes[1].Z) > 0.1]
    return cutter.makeFillet(SOCKET_HOLE_R, edges)


def build_socket_test():
    # Three hole sizes in the same wall thickness as the base. The middle one matches the base.
    plate = box(-33, 33, -8, 8, 0, REAR_WALL)
    cutters = []
    for x, grow in ((-21, -0.4), (0, 0.0), (21, 0.4)):
        cutter = socket_hole(SOCKET_HOLE_W + grow, SOCKET_HOLE_H + grow, REAR_WALL + 2)
        cutter.translate(Vector(x, 0, -1))
        cutters.append(cutter)
    return cut_all(plate, cutters).removeSplitter()


def socket_dummy():
    # The socket's body and spring clips where they sit behind the wall, to check they fit the chamber.
    inner = BASE_REAR - REAR_WALL
    zc = SOCKET_CENTRE_Z
    body = box(-5.95, 5.95, BASE_REAR - 9.1, BASE_REAR, zc - 3.25, zc + 3.25)
    clips = box(-7.6, 7.6, BASE_REAR - 9.1, inner, zc - 2, zc + 2)
    return body.fuse(clips)


def export(shape, name):
    mesh = MeshPart.meshFromShape(Shape=shape, LinearDeflection=0.03, AngularDeflection=math.radians(12), Relative=False)
    mesh.write(os.path.join(OUT, "stl", name + ".stl"))
    box_ = shape.BoundBox
    print("%-12s valid=%s solids=%d closed=%s nonManifold=%s tris=%d vol=%.0fmm3 size=%.1fx%.1fx%.1f" % (
        name, shape.isValid(), len(shape.Solids), mesh.isSolid(), mesh.hasNonManifolds(), mesh.CountFacets,
        shape.Volume, box_.XLength, box_.YLength, box_.ZLength))
    return mesh


def placed(shape, z):
    copy = shape.copy()
    copy.translate(Vector(0, 0, z))
    return copy


def lid_assembled(lid):
    # Print orientation has the outer face at z=0, so flip it about X onto the back of the housing.
    copy = lid.copy()
    copy.Placement = App.Placement(Vector(0, 0, TOTAL_DEPTH), App.Rotation(Vector(1, 0, 0), 180))
    return copy


def transformed(shape, matrix):
    copy = shape.copy()
    copy.transformShape(matrix)
    return copy


def check_interference(parts):
    names = list(parts)
    worst = 0.0
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            volume = parts[a].common(parts[b]).Volume
            worst = max(worst, volume)
            if volume > 0.05:
                print("INTERFERENCE %s / %s: %.2f mm3" % (a, b, volume))
    print("Interference check done, worst overlap %.3f mm3" % worst)


# ---- Preview renderer ---------------------------------------------------------

def tessellate(shape, part_id, transform):
    points, facets = shape.tessellate(0.08)
    vertices = np.array([[p.x, p.y, p.z] for p in points], dtype=float)
    return transform(vertices), np.array(facets, dtype=int), vertices, part_id


def time_pixels():
    # 12:26 with 13 grains, laid out like the firmware.
    font = {"1": ["010", "110", "010", "010", "111"], "2": ["111", "001", "111", "100", "111"], "6": ["111", "100", "111", "101", "111"]}
    lit = {}
    for digit, left in zip("1226", (0, 4, 9, 13)):
        for y, line in enumerate(font[digit]):
            for col, bit in enumerate(line):
                if bit == "1":
                    lit[(left + col, y)] = (1.0, 0.94, 0.84)
    for x in (7, 8):
        for y in (1, 3):
            lit[(x, y)] = (0.42, 0.33, 0.16)
    order = [(7 - d, 7) for d in range(0)]
    for d in range(8):
        order += [(7 - d, 7), (8 + d, 7)]
    for i, cell in enumerate(order[:13]):
        lit[cell] = (1.0, 0.59, 0.12) if (i // 5) % 2 == 0 else (1.0, 0.84, 0.43)
    return lit


def render(items, path, azimuth, elevation, distance, lit_panel, width=1500, height=1000, look=(0, 25, 42)):
    scale = 2
    w, h = width * scale, height * scale
    az, el = math.radians(azimuth), math.radians(elevation)
    eye = np.array(look) + distance * np.array([math.sin(az) * math.cos(el), -math.cos(az) * math.cos(el), math.sin(el)])
    forward = np.array(look) - eye
    forward /= np.linalg.norm(forward)
    right = np.cross(forward, [0, 0, 1.0])
    right /= np.linalg.norm(right)
    up = np.cross(right, forward)
    focal = 2.2 * h / 2
    colors = {0: (0.075, 0.08, 0.09), 1: (0.05, 0.055, 0.06), 2: (0.88, 0.87, 0.82), 3: (0.075, 0.08, 0.09), 4: (0.86, 0.66, 0.38)}
    zbuffer = np.full((h, w), np.inf)
    image = np.zeros((h, w, 3))
    emissive = np.zeros((h, w, 3))
    y_grad = np.linspace(0, 1, h)[:, None, None]
    background = (1 - y_grad) * np.array([0.13, 0.21, 0.24]) + y_grad * np.array([0.05, 0.09, 0.11])
    image[:] = background
    light = np.array([-0.45, -0.75, 0.75])
    light /= np.linalg.norm(light)
    lit = time_pixels()
    xs, ys = cell_centres()
    for world, facets, local, part_id in items:
        rel = world - eye
        cam = np.stack([rel @ right, rel @ up, rel @ forward], axis=1)
        sx = w / 2 + focal * cam[:, 0] / cam[:, 2]
        sy = h / 2 - focal * cam[:, 1] / cam[:, 2]
        base_color = np.array(colors[part_id])
        for a, b, c in facets:
            tri_x = sx[[a, b, c]]
            tri_y = sy[[a, b, c]]
            x0, x1 = max(int(tri_x.min()), 0), min(int(tri_x.max()) + 1, w - 1)
            y0, y1 = max(int(tri_y.min()), 0), min(int(tri_y.max()) + 1, h - 1)
            if x0 > x1 or y0 > y1:
                continue
            denom = (tri_y[1] - tri_y[2]) * (tri_x[0] - tri_x[2]) + (tri_x[2] - tri_x[1]) * (tri_y[0] - tri_y[2])
            if abs(denom) < 1e-9:
                continue
            gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
            l0 = ((tri_y[1] - tri_y[2]) * (gx - tri_x[2]) + (tri_x[2] - tri_x[1]) * (gy - tri_y[2])) / denom
            l1 = ((tri_y[2] - tri_y[0]) * (gx - tri_x[2]) + (tri_x[0] - tri_x[2]) * (gy - tri_y[2])) / denom
            l2 = 1 - l0 - l1
            mask = (l0 >= 0) & (l1 >= 0) & (l2 >= 0)
            if not mask.any():
                continue
            inv_z = l0 / cam[a, 2] + l1 / cam[b, 2] + l2 / cam[c, 2]
            depth = 1 / np.where(inv_z > 0, inv_z, 1e-9)
            region = zbuffer[y0:y1 + 1, x0:x1 + 1]
            visible = mask & (depth < region)
            if not visible.any():
                continue
            region[visible] = depth[visible]
            normal = np.cross(world[b] - world[a], world[c] - world[a])
            length = np.linalg.norm(normal)
            if length == 0:
                continue
            normal /= length
            if normal @ (eye - world[a]) < 0:
                normal = -normal
            diffuse = max(normal @ light, 0.0)
            shade = 0.36 + 0.86 * diffuse
            specular = max(normal @ ((light + (eye - world[a]) / np.linalg.norm(eye - world[a])) / 2), 0.0) ** 40
            rgb = base_color * shade + 0.12 * specular
            target = image[y0:y1 + 1, x0:x1 + 1]
            target[visible] = rgb
            if part_id == 2 and lit_panel:
                lx = (l0 * local[a, 0] + l1 * local[b, 0] + l2 * local[c, 0]) / 1.0
                ly = (l0 * local[a, 1] + l1 * local[b, 1] + l2 * local[c, 1]) / 1.0
                vis_y, vis_x = np.nonzero(visible)
                px, py = lx[vis_y, vis_x], ly[vis_y, vis_x]
                # The viewer's left is the body's +x, so column 0 is the highest x.
                pixels = np.zeros((len(px), 3))
                col_centres = np.array(xs)[::-1]
                row_centres = np.array(ys)[::-1]
                col = np.argmin(np.abs(px[:, None] - col_centres[None, :]), axis=1)
                row = np.argmin(np.abs(py[:, None] - row_centres[None, :]), axis=1)
                du = np.abs(px - col_centres[col]) / (LED_PITCH / 2)
                dv = np.abs(py - row_centres[row]) / (LED_PITCH / 2)
                falloff = np.clip(1.12 - 0.55 * (du ** 2 + dv ** 2), 0, 1)
                walls = np.where((du > 0.9) | (dv > 0.9), 0.5, 1.0)
                for (cx, cy), color in lit.items():
                    hit = (col == cx) & (row == cy)
                    pixels[hit] = np.array(color) * (falloff[hit] * walls[hit])[:, None] * 1.25
                off = np.array([0.075, 0.075, 0.07]) * walls[:, None]
                target_rows = (vis_y, vis_x)
                target[target_rows] = np.clip(off + pixels, 0, 1.4)
                emissive[y0:y1 + 1, x0:x1 + 1][target_rows] = pixels
    glow = emissive.copy()
    for radius in (10, 26):
        blurred = glow
        for axis in (0, 1):
            pad = [(0, 0)] * 3
            pad[axis] = (radius + 1, radius)
            cumulative = np.cumsum(np.pad(blurred, pad, mode="constant"), axis=axis)
            blurred = (np.take(cumulative, range(2 * radius + 1, cumulative.shape[axis]), axis=axis)
                       - np.take(cumulative, range(0, cumulative.shape[axis] - 2 * radius - 1), axis=axis)) / (2 * radius + 1)
        image += blurred * (0.2 if radius == 10 else 0.12)
    image = np.clip(image, 0, 1) ** (1 / 1.25)
    pixels = (image.reshape(h // 2, 2, w // 2, 2, 3).mean(axis=(1, 3)) * 255).astype(np.uint8)
    write_png(path, pixels)


def write_png(path, rgb):
    height, width, _ = rgb.shape
    raw = b"".join(b"\x00" + rgb[y].tobytes() for y in range(height))
    def chunk(tag, data):
        return len(data).to_bytes(4, "big") + tag + data + zlib.crc32(tag + data).to_bytes(4, "big")
    header = width.to_bytes(4, "big") + height.to_bytes(4, "big") + bytes([8, 2, 0, 0, 0])
    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def main():
    os.makedirs(os.path.join(OUT, "stl"), exist_ok=True)
    os.makedirs(os.path.join(OUT, "preview"), exist_ok=True)
    print("Outer screen size %.1f x %.1f x %.1f, seam offset %.2f" % (W, H, TOTAL_DEPTH, SEAM))

    housing = build_housing()
    diffuser = build_diffuser()
    grid = build_grid()
    lid = build_lid()
    base = build_base()

    export(housing, "1_housing")
    export(grid, "2_light_grid")
    export(diffuser, "3_diffuser_WHITE")
    export(lid, "4_rear_lid")
    export(base, "5_dune_base")
    export(build_socket_test(), "6_socket_fit_test")
    print("Socket overlap with base %.3f mm3" % socket_dummy().common(base).Volume)
    Part.makeCompound([housing, grid, diffuser, lid, base]).exportStep(os.path.join(OUT, "sandbar_case_parts.step"))

    matrix = body_to_base_matrix()
    pcb = box(-PCB_WIDTH, PCB_WIDTH, -ARRAY_HALF_H - 0.5, ARRAY_HALF_H + 0.5, PCB_FRONT_Z, PCB_BACK_Z)
    assembled_lid = lid_assembled(lid)
    check_interference({
        "housing": housing, "diffuser": placed(diffuser, FRONT_THICKNESS), "grid": placed(grid, GRID_Z),
        "lid": assembled_lid, "pcb": pcb, "base": transformed(base, matrix.inverse()),
    })

    def in_base(shift):
        def apply(vertices):
            v = vertices + np.array([0, 0, shift])
            s, c = math.sin(THETA), math.cos(THETA)
            return np.stack([-v[:, 0], v[:, 1] * s + v[:, 2] * c + H / 2 * s,
                             v[:, 1] * c - v[:, 2] * s + BASE_FLOOR_V + H / 2 * c], axis=1)
        return apply

    def lid_points(vertices):
        return np.stack([vertices[:, 0], -vertices[:, 1], TOTAL_DEPTH - vertices[:, 2]], axis=1)

    def items(diffuser_z, grid_z, lid_shift, housing_shift, base_shift):
        def lid_transform(vertices):
            return in_base(lid_shift)(lid_points(vertices))
        def base_transform(vertices):
            return vertices + np.array([0, 0, base_shift])
        return [
            tessellate(housing, 0, in_base(housing_shift)), tessellate(grid, 1, in_base(grid_z)),
            tessellate(diffuser, 2, in_base(diffuser_z)), tessellate(lid, 3, lid_transform), tessellate(base, 4, base_transform),
        ]

    assembled = items(FRONT_THICKNESS, GRID_Z, 0, 0, 0)
    render(assembled, os.path.join(OUT, "preview", "front.png"), 24, 12, 270, True)
    render(assembled, os.path.join(OUT, "preview", "rear.png"), 150, 18, 270, False)
    exploded = items(FRONT_THICKNESS - 45, GRID_Z - 15, 40, -80, -28)
    render(exploded, os.path.join(OUT, "preview", "exploded.png"), 42, 16, 400, False, look=(0, 0, 38))
    print("Done")


main()
sys.stdout.flush()
os._exit(0)
