#!/usr/bin/env python3
import json
import sys
from pathlib import Path
import matplotlib.pyplot as plt


def load_tree(path):
    with open(path, 'r') as f:
        return json.load(f)





def layout(node, depth=0, positions=None, widths=None, x_step=1.8):
    # positions: dict name->(x,y)
    if positions is None:
        positions = {}
    if widths is None:
        widths = {'next_x': 0}

    name = node.get('name', 'node')
    children = node.get('children', [])
    if not children:
        x = widths['next_x'] * x_step
        positions[name] = (x, -depth)
        widths['next_x'] += 1
        return positions

    # layout children first
    child_positions = []
    for c in children:
        layout(c, depth + 1, positions, widths, x_step)
        child_positions.append(positions[c.get('name')])

    # parent x is mean of children
    xs = [p[0] for p in child_positions]
    px = sum(xs) / len(xs)
    positions[name] = (px, -depth)
    return positions


def compute_totals(node, totals=None):
    if totals is None:
        totals = {}
    name = node.get('name')
    children = node.get('children', [])
    if not children:
        total = float(node.get('amps', 0.0))
        totals[name] = total
        return total
    s = 0.0
    for c in children:
        s += compute_totals(c, totals)
    totals[name] = s
    return s


def get_display_label(node):
    name = node.get('name', 'node')
    if 'label' in node and node.get('label'):
        return str(node['label'])
    if isinstance(name, str) and '_' in name:
        return name.split('_', 1)[0]
    return name


def build_label_map(node, labels=None):
    if labels is None:
        labels = {}
    name = node.get('name')
    if name is not None:
        labels[name] = get_display_label(node)
    for child in node.get('children', []):
        build_label_map(child, labels)
    return labels


def draw(tree_path, out_path):
    tree = load_tree(tree_path)
    positions = layout(tree)
    totals = {}
    compute_totals(tree, totals)
    labels = build_label_map(tree)

    # compute layout extents and scale figure to reduce overlap
    xs = [p[0] for p in positions.values()]
    ys = [p[1] for p in positions.values()]
    max_x = max(xs) if xs else 1
    max_depth = max([-y for y in ys]) if ys else 1
    fig_width = max(10, min(16, max_x * 0.9 + 2))
    fig_height = max(7, min(10, max_depth * 1.0 + 1))
    fig, ax = plt.subplots(figsize=(fig_width, fig_height))

    # draw edges
    non_leaves = set()
    def draw_edges(node):
        name = node.get('name')
        children = node.get('children', [])
        if children:
            non_leaves.add(name)
        for c in children:
            child_name = c.get('name')
            x0, y0 = positions[name]
            x1, y1 = positions[child_name]
            ax.plot([x0, x1], [y0, y1], color='gray')
            amp = totals.get(child_name, 0.0)
            mid_x = (x0 + x1) / 2
            mid_y = (y0 + y1) / 2
            ax.text(mid_x + 0.12, mid_y, f"{amp:.1f}A", va='center', ha='left', fontsize=6, color='dimgray')
            draw_edges(c)

    draw_edges(tree)

    # draw nodes; leaf nodes (not in non_leaves) are orange
    # offset labels slightly to the right to avoid overlapping node markers
    for n, (x, y) in positions.items():
        color = 'orange' if n not in non_leaves else 'tab:blue'
        ax.scatter(x, y, s=180, color=color, edgecolor='black')
        amp = totals.get(n, 0.0)
        label_text = labels.get(n, n)
        label = f"{label_text} ({amp:.1f}A)"
        ax.text(x, y - 0.18, label, va='top', ha='center', fontsize=7, clip_on=False)

    ax.set_axis_off()
    ax.set_xlim(-1, max_x + 2)
    ax.set_ylim(min(ys) - 1, 1)
    fig.tight_layout()
    out_dir = Path(out_path).parent
    out_dir.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=150)
    print('Wrote', out_path)


if __name__ == '__main__':
    tree_path = sys.argv[1] if len(sys.argv) > 1 else 'validate/house_tree.json'
    out_path = sys.argv[2] if len(sys.argv) > 2 else 'validate/house_tree.png'
    draw(tree_path, out_path)
