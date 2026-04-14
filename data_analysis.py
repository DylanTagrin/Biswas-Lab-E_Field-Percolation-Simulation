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
    line_artists = []

    for e in electrodes:
        shape = e["shape"]

        if shape == "triangle":
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

        elif shape == "needle":
            v1 = np.array(e["v1"], dtype=float)
            v2 = np.array(e["v2"], dtype=float)
            v3 = np.array(e["v3"], dtype=float)
            cp = np.array(e["cp"], dtype=float)

            t = np.linspace(0.0, 1.0, 200)

            # quadratic Bezier from v1 to v3 with control point cp
            curve1 = ((1 - t)[:, None] ** 2) * v1 + 2 * ((1 - t)[:, None] * t[:, None]) * cp + (t[:, None] ** 2) * v3

            # quadratic Bezier from v2 to v3 with control point cp
            curve2 = ((1 - t)[:, None] ** 2) * v2 + 2 * ((1 - t)[:, None] * t[:, None]) * cp + (t[:, None] ** 2) * v3

            # straight edge from v1 to v2
            edge = np.vstack([v1, v2])

            line1, = ax.plot(curve1[:, 0], curve1[:, 1], linewidth=2.0)
            line2, = ax.plot(curve2[:, 0], curve2[:, 1], linewidth=2.0)
            line3, = ax.plot(edge[:, 0], edge[:, 1], linewidth=2.0)

            line_artists.extend([line1, line2, line3])
    return patches, line_artists


def animate_circles(data, output_path="output/circles_animation.mp4"):
    width = data["width"]
    height = data["height"]

    moving = data["moving_circles"]
    static = data["static_circles"]
    moving_chains = data.get("moving_chains", [])
    static_chains = data.get("static_chains", [])
    electrodes = data.get("electrodes", [])

    n_frames = min(
        len(moving),
        len(static),
        len(moving_chains) if moving_chains else len(moving),
        len(static_chains) if static_chains else len(static),
    )

    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_xlim(0, width)
    ax.set_ylim(0, height)
    ax.set_aspect("equal")
    ax.set_title("Circle Positions vs Time")
    ax.set_xlabel("x")
    ax.set_ylabel("y")

    electrode_patches, electrode_lines = draw_electrodes(ax, electrodes)

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

    def add_circle_patch(circle_dict, linestyle="-", linewidth=1.5):
        e = Ellipse(
            xy=(circle_dict["x"], circle_dict["y"]),
            width=2 * circle_dict["a"],
            height=2 * circle_dict["b"],
            fill=False,
            linewidth=linewidth,
            linestyle=linestyle,
        )
        ax.add_patch(e)
        dynamic_patches.append(e)

    def update(frame_idx):
        clear_dynamic()

        # Free moving circles
        for c in moving[frame_idx]:
            add_circle_patch(c, linestyle="-", linewidth=1.5)

        # Free static circles
        for c in static[frame_idx]:
            add_circle_patch(c, linestyle="--", linewidth=2.0)

        # Moving chain members
        for chain in moving_chains[frame_idx]:
            for member in chain["members"]:
                add_circle_patch(member, linestyle="-", linewidth=1.5)

        # Static chain members
        for chain in static_chains[frame_idx]:
            for member in chain["members"]:
                add_circle_patch(member, linestyle="--", linewidth=2.0)

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
                         stride=20):
    field_x = data["field_x"]
    field_y = data["field_y"]
    electrodes = data.get("electrodes", [])

    n_frames, height, width = field_x.shape

    y, x = np.mgrid[0:height:stride, 0:width:stride]

    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_xlim(0, width)
    ax.set_ylim(0, height)
    ax.set_aspect("equal")
    ax.set_title("Electric Field Direction vs Time")
    ax.set_xlabel("x")
    ax.set_ylabel("y")

    draw_electrodes(ax, electrodes)

    def normalize(u, v, eps=1e-12):
        mag = np.sqrt(u**2 + v**2)

        u_norm = np.zeros_like(u, dtype=float)
        v_norm = np.zeros_like(v, dtype=float)

        np.divide(u, mag, out=u_norm, where=mag > eps)
        np.divide(v, mag, out=v_norm, where=mag > eps)

        return u_norm, v_norm

    u0 = field_x[0, ::stride, ::stride]
    v0 = field_y[0, ::stride, ::stride]
    u0, v0 = normalize(u0, v0)

    q = ax.quiver(
        x, y, u0, v0,
        angles="xy",
        scale_units="xy",
        scale=0.07,      # smaller scale = longer arrows
        pivot="mid",
        width=0.0025
    )

    title = ax.text(
        0.02, 0.98, "", transform=ax.transAxes,
        ha="left", va="top"
    )

    def update(frame_idx):
        u = field_x[frame_idx, ::stride, ::stride]
        v = field_y[frame_idx, ::stride, ::stride]
        u, v = normalize(u, v)

        q.set_UVC(u, v)
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


def split_relax_runs(relax_iterations, relax_error):
    """
    Split flat relax history into one list per relaxation call.
    A new run starts whenever iteration count drops.
    """
    if len(relax_iterations) != len(relax_error):
        raise ValueError("relax_iterations and relax_error must have the same length")

    runs = []
    current_iters = []
    current_errs = []

    prev_iter = None
    for it, err in zip(relax_iterations, relax_error):
        if prev_iter is not None and it < prev_iter:
            runs.append((current_iters, current_errs))
            current_iters = []
            current_errs = []

        current_iters.append(it)
        current_errs.append(err)
        prev_iter = it

    if current_iters:
        runs.append((current_iters, current_errs))

    return runs


def plot_relax_error_decay(data, output_path="output/relax_error_decay.png",
                           max_runs=None):
    relax_iterations = data.get("relax_iterations", [])
    relax_error = data.get("relax_error", [])

    if not relax_iterations or not relax_error:
        print("No relax history found.")
        return

    runs = split_relax_runs(relax_iterations, relax_error)

    if max_runs is not None:
        runs = runs[:max_runs]

    fig, ax = plt.subplots(figsize=(8, 5))

    for run_idx, (iters, errs) in enumerate(runs):
        if len(iters) == 0:
            continue
        ax.plot(iters, errs, label=f"Run {run_idx}")

    ax.set_title("Relative Error Decay per Relaxation Run")
    ax.set_xlabel("Relaxation Iteration")
    ax.set_ylabel("Relative Error")
    ax.set_yscale("log")
    ax.legend(fontsize=8, ncol=2)
    fig.tight_layout()
    fig.savefig(output_path, dpi=150)
    plt.close(fig)


def animate_field_quiver_unnormalized(data, output_path="output/efield_quiver_animation.mp4",
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

def main():
    data = load_sim_data(DATA_PATH)

    # animate_circles(data)
    # animate_potential(data)
    animate_field_quiver(data, stride=20)
    # animate_field_quiver_unnormalized(data, stride=25)
    # plot_relax_error_decay(data)
    # plot_resistance_vs_time(data)


if __name__ == "__main__":
    main()