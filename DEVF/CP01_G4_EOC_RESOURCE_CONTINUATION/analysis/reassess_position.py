#!/usr/bin/env python3
"""Incremental KCL query for the NEW resource radius at the old reference only.

Reuses CP01 metadata/half-plane/support readers. Visits only octree cells
intersecting the new box and decodes their prism references, never whole KCL.
No new spawn position, actor, layer, flags, or runtime operation.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parents[1]
CP01 = HERE.parent / "CP01"
sys.path.insert(0, str(CP01 / "analysis"))
REFERENCE = [-4806.7346, -6729.480961460304, 65135.8281]


def overlap(lo, hi, other_lo, other_hi):
    return all(lo[i] <= other_hi[i] and hi[i] >= other_lo[i] for i in range(3))


def point_triangle_distance(p, triangle):
    sub = lambda a,b: tuple(x-y for x,y in zip(a,b))
    dot = lambda a,b: sum(x*y for x,y in zip(a,b))
    cross = lambda a,b: (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    a,b,c = triangle
    normal = cross(sub(b,a),sub(c,a)); nn=dot(normal,normal)
    assert nn > 0
    t=dot(sub(p,a),normal)/nn
    projection=tuple(p[i]-t*normal[i] for i in range(3))
    inside=all(dot(cross(sub(y,x),sub(projection,x)),normal)>=-1e-6 for x,y in [(a,b),(b,c),(c,a)])
    if inside:
        return abs(t)*math.sqrt(nn)
    distances=[]
    for x,y in [(a,b),(b,c),(c,a)]:
        edge=sub(y,x);length=dot(edge,edge)
        f=max(0,min(1,dot(sub(p,x),edge)/length)) if length else 0
        distances.append(math.dist(p,tuple(x[i]+f*edge[i] for i in range(3))))
    return min(distances)


def query_cells(data, lo, hi, u):
    _, _, _, octree = u(data, 0, "IIII")
    minimum = u(data, 20, "fff")
    masks = u(data, 32, "III")
    shift, sx, sxy = u(data, 44, "III")
    dims = [(((~m) & 0xffffffff) >> shift) + 1 for m in masks]
    width = 1 << shift
    ids, leaves = set(), set()
    visits = 0
    def walk(base, offset, low, w, level, trail=()):
        nonlocal visits
        high = [a + w for a in low]
        if not overlap(lo, hi, low, high):
            return
        visits += 1
        value = u(data, base + offset, "I")[0]
        if value & 0x80000000:
            leaf = base + (value & 0x7fffffff)
            if leaf not in leaves:
                leaves.add(leaf)
                cursor = leaf + 2
                while True:
                    index = u(data, cursor, "H")[0]; cursor += 2
                    if index == 0:
                        break
                    ids.add(index)
            return
        assert level > 0
        child = base + value
        assert child not in trail
        half = w // 2
        for i in range(8):
            child_low = [low[a] + (((i >> a) & 1) * half) for a in range(3)]
            walk(child, i * 4, child_low, half, level - 1, trail + (child,))
    for z in range(dims[2]):
        for y in range(dims[1]):
            for x in range(dims[0]):
                root_low = [minimum[i] + v * width for i, v in enumerate((x, y, z))]
                walk(octree, 4 * ((z << sxy) | (y << sx) | x), root_low, width, shift)
    return sorted(ids), {"visited_intersecting_cells": visits, "leaf_lists": len(leaves), "prisms_decoded": len(ids)}


def selected_prisms(data, plc, indices, u, dot, cross, add, mul):
    op, on, ot, ob = u(data, 0, "IIII")
    total = (ob - ot) // 16 - 1
    result = []
    for index in indices:
        assert 1 <= index <= total
        height, vi, ni, a, b, c, attribute = u(data, ot + index * 16, "fHHHHHH")
        p = u(data, op + vi * 12, "fff")
        normal = lambda n: u(data, on + n * 12, "fff")
        n, e1, e2, e3 = normal(ni), normal(a), normal(b), normal(c)
        r1, r2 = cross(n, e1), cross(e2, n)
        points = [p, add(p, mul(r2, height/dot(r2,e3))), add(p, mul(r1, height/dot(r1,e3)))]
        result.append({"index": index, "normal": n, "edges": [e1,e2,e3], "pts": points,
                       "height": height, "attribute": attribute, "code": u(plc, 8 + attribute * 20, "IIIII")})
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--location-cache", required=True, type=Path)
    ap.add_argument("--envelope", required=True, type=Path)
    ap.add_argument("--output", required=True, type=Path)
    args = ap.parse_args()
    os.environ["DEVF_CP01_LOCATION_WORK"] = str(args.location_cache)
    from analyze_location import u
    from location_geometry import dot, cross, add, mul, hit, plc_dec, area
    from verify_footprint import constraints, halfclip, subtract
    from location_spatial import triangle_box
    envelope = json.loads(args.envelope.read_text())
    radius = envelope["resource_skeletal_bound"]["combined_radius_upper"]
    low, high = [v-radius for v in REFERENCE], [v+radius for v in REFERENCE]
    kcl = (args.location_cache / "R03_00.arc/kcl/room.kcl").read_bytes()
    plc = (args.location_cache / "R03_00.arc/plc/room.plc").read_bytes()
    ids, counts = query_cells(kcl, low, high, u)
    triangles = selected_prisms(kcl, plc, ids, u, dot, cross, add, mul)
    previous = json.loads((CP01 / "evidence/location-f-sp121-summary.json").read_text())
    assert len(triangles) < previous["room_kcl"]["prism_count"]
    queries = {}
    for label, r in [("acch100_square",100), ("resource501_square",radius), ("resource501_circumscribed64",radius)]:
        if label.endswith("circumscribed64"):
            # Faces tangent to the radius-r circle; vertices fit the queried square.
            outer = r/math.cos(math.pi/64)
            poly=[(REFERENCE[0]+outer*math.cos((2*i+1)*math.pi/64),REFERENCE[2]+outer*math.sin((2*i+1)*math.pi/64)) for i in range(64)]
        else:
            poly = [(REFERENCE[0]-r,REFERENCE[2]-r), (REFERENCE[0]+r,REFERENCE[2]-r),
                    (REFERENCE[0]+r,REFERENCE[2]+r), (REFERENCE[0]-r,REFERENCE[2]+r)]
        remainder, contributors, heights, slopes, plcs = [poly], [], [], [], []
        for t in triangles:
            # Same ground eligibility as the existing CP01 support reader.
            if t["normal"][1] < 0.5 or t["code"][0] & (1 << 14) or t["code"][4] & (1 << 8):
                continue
            clipped = poly
            for a,b,c in constraints(t):
                clipped = halfclip(clipped,a,b,c)
            if area(clipped) <= 1e-7:
                continue
            n,p = t["normal"],t["pts"][0]
            ys = [p[1]-((x-p[0])*n[0]+(z-p[2])*n[2])/n[1] for x,z in clipped]
            if max(ys) > REFERENCE[1]+radius or min(ys) < REFERENCE[1]-radius:
                continue  # no proof for surfaces outside queried Y
            heights += ys
            slopes.append(math.degrees(math.acos(max(-1,min(1,n[1])))))
            plcs.append(plc_dec(t["code"]))
            contributors.append(t["index"])
            remainder = [r for q in remainder for r in subtract(q,constraints(t))]
        queries[label] = {"radius": r, "area": area(poly), "uncovered_ground_area": sum(area(p) for p in remainder),
                          "ground_prisms": contributors, "terrain_height_minmax": [min(heights),max(heights)] if heights else None,
                          "max_slope_degrees": max(slopes) if slopes else None,
                          "plc_ground_codes": sorted(set(p["ground_code"] for p in plcs)), "plc_rooms": sorted(set(p["room"] for p in plcs)),
                          "scope": "Union of eligible projected ground prisms, not feet contact/connected traversability/no-water proof"}
    supports = [(h,t) for t in triangles if (h:=hit(t,REFERENCE[0],REFERENCE[2])) is not None and h < REFERENCE[1]+100]
    supports.sort(key=lambda value:value[0],reverse=True)
    walls = [{"prism":t["index"],"distance_to_reference_triangle":point_triangle_distance(REFERENCE,t["pts"]),
              "distance_minus_resource_radius":point_triangle_distance(REFERENCE,t["pts"])-radius}
             for t in triangles if t["normal"][1] < 0.5 and triangle_box(t["pts"],low,high)]
    extra = json.loads((CP01 / "evidence/extra-collisions-verification.json").read_text())
    old_box = extra["candidate_box"]
    assert all(old_box[0][i] <= REFERENCE[i] <= old_box[1][i] for i in range(3))
    external = []
    for instance in extra["instances"]:
        p = instance["physics"]
        old_gap = p["aabb_distance_lower_bound"]
        external.append({"name":instance["name"],"chunk":instance["chunk"],"index":instance["index"],
                         "distance_lower_from_old_box":old_gap,"fixed_origin_resource_bound_gap_lower":old_gap-radius,
                         "condition":"Triangle inequality from prior box containing origin; same authored transforms; excludes displacement"})
    g2path = HERE.parent / "CP01_G4_EOC_SAFETY/evidence/g4-aggregates.json"
    g2 = json.loads(g2path.read_text())
    dig = json.loads((CP01 / "evidence/location-f-sp121-digpoints.json").read_text())
    result = {"scope":"New501 resource enclosure at old reference only; targeted octree query, no full KCL reconstruction",
              "reference_only": {"xyz":REFERENCE,"yaw":0,"stage":"F_SP121","room":3,"logical":6,"status":"NOT_APPROVED_FOR_E_OC"},
              "query_box": [low,high], "query_counts": counts, "ground_queries":queries,
              "center_support":{"prism":supports[0][1]["index"],"y":supports[0][0],"normal":supports[0][1]["normal"],"plc":plc_dec(supports[0][1]["code"])} if supports else None,
              "wall_roof_box_intersections":walls,"external_fixed_origin_separations":external,
              "reused_g2":{"sha256":hashlib.sha256(g2path.read_bytes()).hexdigest(),"captures":28,"link_min_center_xz":g2["reference_only"]["min_sampled_link_center_xz"],
                           "link_distance_minus_resource_radius":g2["reference_only"]["min_sampled_link_center_xz"]-radius,
                           "limit":"Conservative ball overlaps sampled Link center; not proof of an actual collision/attack"},
              "reused_digpl":{"points":27,"old_box_gap":dig["nearest_dig_point"]["distance_to_model_rectangle_xz"],
                              "center_point_distance":dig["nearest_dig_point"]["center_distance_xz"],"new_ball_point_gap_lower":max(0,dig["nearest_dig_point"]["center_distance_xz"]-radius),
                              "limit":"Spatial comparison only; Digpl consumes player observations, not an E_OC sweep"},
              "reused_route7_polyline_distance_to_old_box":115.615565,
              "movement_domain":"NOT_VERIFIED; no claim that2000+501 is a hard roaming domain",
              "effective_resource_override":"NOT_VERIFIED in supplied manifest",
              "position":"NO_SAFE_POSITION_VERIFIED","g4":"G4_RESOURCE_VERIFIED_POSITION_BLOCKED","g5":"NO-GO","cp02":"NO-GO","policy":"DISARMED_NO_REGISTRATION"}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps({"query_counts":counts,"ground_queries":queries,"center_support":result["center_support"],"wall_roof_count":len(walls)},indent=2))


if __name__ == "__main__":
    main()
