import json
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.patches import Ellipse, Polygon, Rectangle


DATA_PATH = Path("output/data.json")
FPS = 5


def load_sim_data(path: Path):
    with path.open("r") as f:
        j = json.load(f)

    width = int(j["width"])
    height = int(j["height"])

    out = {
        "width": width,
        "height": height,
        "moving_circles": j.get("moving_circles", []),
        "static_circles": j.get("static_circles", []),
        "moving_chains": j.get("moving_chains", []),
        "static_chains": j.get("static_chains", []),
        "resistance_measurements": j.get("resistance_measurements", []),
        "relax_iterations": j.get("relax_iterations", []),
        "relax_error": j.get("relax_error", []),
        "electrode_type": j.get("electrode_type", None),
        "electrodes": j.get("electrodes", []),
    }

    if "potential" in j:
        potential = np.asarray(j["potential"], dtype=float)
        out["potential"] = potential.reshape(len(potential), height, width)

    if "field_mag" in j:
        field_mag = np.asarray(j["field_mag"], dtype=float)
        out["field_mag"] = field_mag.reshape(len(field_mag), height, width)

    if "field_x" in j:
        field_x = np.asarray(j["field_x"], dtype=float)
        out["field_x"] = field_x.reshape(len(field_x), height, width)

    if "field_y" in j:
        field_y = np.asarray(j["field_y"], dtype=float)
        out["field_y"] = field_y.reshape(len(field_y), height, width)

    return out


def draw_electrodes(ax, electrodes):
    patches = []
    for e in electrodes:
        shape = e["shape"]

        if shape in ("triangle", "needle"):
            pts = np.array([e["v1"], e["v2"], e["v3"]], dtype=float)
            poly = Polygon(pts, closed=True, fill=False, linewidth=2.0)
            ax.add_patch(poly)
            patches.append(poly)

        elif shape == "rectangle":
            pos = e["position"]
            size = e["size"]
            rect = Rectangle(
                xy=(pos[0], pos[1]),
                width=size[0],
                height=size[1],
                fill=False,
                linewidth=2.0,
            )
            ax.add_patch(rect)
            patches.append(rect)

    return patches


def animate_circles(data, output_path="output/circles_animation.mp4"):
    width = data["width"]
    height = data["height"]
    moving = data["moving_circles"]
    static = data["static_circles"]
    electrodes = data.get("electrodes", [])

    n_frames = min(len(moving), len(static))

    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_xlim(0, width)
    ax.set_ylim(0, height)
    ax.set_aspect("equal")
    ax.set_title("Circle Positions vs Time")
    ax.set_xlabel("x")
    ax.set_ylabel("y")

    # Uncomment if the geometry looks vertically flipped
    # ax.invert_yaxis()

    draw_electrodes(ax, electrodes)

    dynamic_patches = []
    title = ax.text(
        0.02, 0.98, "", transform=ax.transAxes,
        ha="left", va="top"
    )

    def clear_dynamic():
        nonlocal dynamic_patches
        for p in dynamic_patches:
            p.remove()
        dynamic_patches = []

    def update(frame_idx):
        clear_dynamic()

        for c in moving[frame_idx]:
            e = Ellipse(
                xy=(c["x"], c["y"]),
                width=2 * c["a"],
                height=2 * c["b"],
                fill=False,
                linewidth=1.5,
            )
            ax.add_patch(e)
            dynamic_patches.append(e)

        for c in static[frame_idx]:
            e = Ellipse(
                xy=(c["x"], c["y"]),
                width=2 * c["a"],
                height=2 * c["b"],
                fill=False,
                linewidth=2.0,
                linestyle="--",
            )
            ax.add_patch(e)
            dynamic_patches.append(e)

        title.set_text(f"Frame {frame_idx}")
        return dynamic_patches + [title]

    ani = animation.FuncAnimation(
        fig, update, frames=n_frames, interval=1000 // FPS, blit=False
    )
    ani.save(output_path, writer=animation.FFMpegWriter(fps=FPS))
    plt.close(fig)


def animate_potential(data, output_path="output/potential_animation.mp4"):
    potential = data["potential"]
    electrodes = data.get("electrodes", [])
    n_frames, height, width = potential.shape

    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_title("Potential Heat Map vs Time")
    ax.set_xlabel("x")
    ax.set_ylabel("y")

    vmin = np.min(potential)
    vmax = np.max(potential)

    im = ax.imshow(
        potential[0],
        origin="lower",
        aspect="auto",
        vmin=vmin,
        vmax=vmax,
    )
    fig.colorbar(im, ax=ax, label="Potential")

    draw_electrodes(ax, electrodes)

    title = ax.text(
        0.02, 0.98, "", transform=ax.transAxes,
        ha="left", va="top", color="white"
    )

    def update(frame_idx):
        im.set_data(potential[frame_idx])
        title.set_text(f"Frame {frame_idx}")
        return [im, title]

    ani = animation.FuncAnimation(
        fig, update, frames=n_frames, interval=1000 // FPS, blit=False
    )
    ani.save(output_path, writer=animation.FFMpegWriter(fps=FPS))
    plt.close(fig)


def animate_field_quiver(data, output_path="output/efield_quiver_animation.mp4",
                         stride=20, scale=None):
    field_x = data["field_x"]
    field_y = data["field_y"]
    electrodes = data.get("electrodes", [])

    n_frames, height, width = field_x.shape

    y, x = np.mgrid[0:height:stride, 0:width:stride]

    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_xlim(0, width)
    ax.set_ylim(0, height)
    ax.set_aspect("equal")
    ax.set_title("Electric Field Quiver vs Time")
    ax.set_xlabel("x")
    ax.set_ylabel("y")

    draw_electrodes(ax, electrodes)

    q = ax.quiver(
        x, y,
        field_x[0, ::stride, ::stride],
        field_y[0, ::stride, ::stride],
        scale=scale
    )

    title = ax.text(
        0.02, 0.98, "", transform=ax.transAxes,
        ha="left", va="top"
    )

    def update(frame_idx):
        q.set_UVC(
            field_x[frame_idx, ::stride, ::stride],
            field_y[frame_idx, ::stride, ::stride]
        )
        title.set_text(f"Frame {frame_idx}")
        return [q, title]

    ani = animation.FuncAnimation(
        fig, update, frames=n_frames, interval=1000 // FPS, blit=False
    )
    ani.save(output_path, writer=animation.FFMpegWriter(fps=FPS))
    plt.close(fig)


def plot_resistance_vs_time(data, output_path="output/resistance_vs_time.png"):
    r = np.asarray(data.get("resistance_measurements", []), dtype=float)

    if r.size == 0:
        return

    frames = np.arange(len(r))
    fig, ax = plt.subplots(figsize=(8, 4))
    ax.plot(frames, r)
    ax.set_title("Resistance vs Time")
    ax.set_xlabel("Frame")
    ax.set_ylabel("Resistance")
    fig.savefig(output_path, dpi=150)
    plt.close(fig)


def main():
    data = load_sim_data(DATA_PATH)

    animate_circles(data)
    animate_potential(data)

    # Uncomment once you want the field animation too
    # animate_field_quiver(data, stride=25)

    plot_resistance_vs_time(data)


if __name__ == "__main__":
    main()