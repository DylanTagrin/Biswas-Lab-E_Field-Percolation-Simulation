import json
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.colors import LogNorm, Normalize
from matplotlib.patches import Ellipse, Polygon, Rectangle


DATA_PATH = Path("output/data.json")
OUTPUT_DIR = Path("output")
FPS = 5
FIGSIZE = (10, 7)
DPI = 150


# -----------------------------
# Data loading
# -----------------------------

def load_sim_data(path: Path):
    with path.open("r", encoding="utf-8") as f:
        j = json.load(f)

    width = int(j["width"])
    height = int(j["height"])

    data = {
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
        arr = np.asarray(j["potential"], dtype=float)
        data["potential"] = arr.reshape(len(arr), height, width)

    if "field_mag" in j:
        arr = np.asarray(j["field_mag"], dtype=float)
        data["field_mag"] = arr.reshape(len(arr), height, width)

    if "field_x" in j:
        arr = np.asarray(j["field_x"], dtype=float)
        data["field_x"] = arr.reshape(len(arr), height, width)

    if "field_y" in j:
        arr = np.asarray(j["field_y"], dtype=float)
        data["field_y"] = arr.reshape(len(arr), height, width)

    return data


def get_frame_count(data):
    counts = []

    for key in ["potential", "field_mag", "field_x", "field_y"]:
        if key in data:
            counts.append(data[key].shape[0])

    for key in ["moving_circles", "static_circles", "moving_chains", "static_chains"]:
        if data.get(key):
            counts.append(len(data[key]))

    if not counts:
        raise ValueError("No frame-based data found in JSON.")

    return min(counts)


# -----------------------------
# Basic plot helpers
# -----------------------------

def ensure_output_dir(path):
    Path(path).parent.mkdir(parents=True, exist_ok=True)


def setup_axes(ax, width, height, title):
    ax.set_xlim(0, width)
    ax.set_ylim(0, height)
    ax.set_aspect("equal")
    ax.set_title(title)
    ax.set_xlabel("x")
    ax.set_ylabel("y")


def add_frame_label(ax, frame_idx, extra_text=None, color="white"):
    text = f"Frame {frame_idx}"
    if extra_text:
        text += f"\n{extra_text}"
    ax.text(
        0.02,
        0.98,
        text,
        transform=ax.transAxes,
        ha="left",
        va="top",
        color=color,
        bbox=dict(boxstyle="round", facecolor="black", alpha=0.35, edgecolor="none"),
    )


# -----------------------------
# Heatmap tuning
# -----------------------------
# Use these presets directly, or make your own.
# mode:
#   "full"       -> true global min/max across all frames
#   "percentile" -> clip outliers using low/high percentiles
#   "log"        -> log scaling for wide dynamic range

def make_heatmap_settings(mode="percentile", cmap="inferno", low=1, high=99, alpha=1.0):
    return {
        "mode": mode,
        "cmap": cmap,
        "low": low,
        "high": high,
        "alpha": alpha,
    }


def make_norm(stack, settings):
    mode = settings.get("mode", "percentile")
    low = settings.get("low", 1)
    high = settings.get("high", 99)

    values = np.asarray(stack, dtype=float)
    values = values[np.isfinite(values)]

    if values.size == 0:
        return Normalize(vmin=0.0, vmax=1.0, clip=True)

    if mode == "full":
        return Normalize(vmin=float(values.min()), vmax=float(values.max()), clip=True)

    if mode == "percentile":
        vmin = float(np.percentile(values, low))
        vmax = float(np.percentile(values, high))
        if vmax <= vmin:
            vmax = vmin + 1.0
        return Normalize(vmin=vmin, vmax=vmax, clip=True)

    if mode == "log":
        values = values[values > 0]
        if values.size == 0:
            return LogNorm(vmin=1e-12, vmax=1.0, clip=True)
        vmin = max(float(np.percentile(values, low)), 1e-12)
        vmax = max(float(np.percentile(values, high)), vmin * 10.0)
        return LogNorm(vmin=vmin, vmax=vmax, clip=True)

    raise ValueError(f"Unknown heatmap mode: {mode}")


def draw_heatmap(ax, frame2d, norm, settings):
    return ax.imshow(
        frame2d,
        origin="lower",
        aspect="auto",
        cmap=settings.get("cmap", "inferno"),
        norm=norm,
        alpha=settings.get("alpha", 1.0),
        zorder=1,
    )


# -----------------------------
# Geometry drawing
# -----------------------------

def draw_electrodes(ax, electrodes):
    for elec in electrodes:
        value = elec.get("value", 0.0)
        if value > 0:
            color = "red"
        elif value < 0:
            color = "cyan"
        else:
            color = "white"

        shape = elec["shape"]

        if shape == "triangle":
            pts = np.array([elec["v1"], elec["v2"], elec["v3"]], dtype=float)
            ax.add_patch(Polygon(pts, closed=True, fill=False, linewidth=2.0, color=color, alpha=0.9))

        elif shape == "rectangle":
            pos = elec["position"]
            size = elec["size"]
            ax.add_patch(Rectangle((pos[0], pos[1]), size[0], size[1], fill=False,
                                   linewidth=2.0, color=color, alpha=0.9))

        elif shape == "needle":
            v1 = np.array(elec["v1"], dtype=float)
            v2 = np.array(elec["v2"], dtype=float)
            v3 = np.array(elec["v3"], dtype=float)
            cp = np.array(elec["cp"], dtype=float)
            t = np.linspace(0.0, 1.0, 300)
            curve1 = ((1 - t)[:, None] ** 2) * v1 + 2 * ((1 - t)[:, None] * t[:, None]) * cp + (t[:, None] ** 2) * v3
            curve2 = ((1 - t)[:, None] ** 2) * v2 + 2 * ((1 - t)[:, None] * t[:, None]) * cp + (t[:, None] ** 2) * v3
            edge = np.vstack([v1, v2])
            ax.plot(curve1[:, 0], curve1[:, 1], linewidth=2.0, color=color, alpha=0.9)
            ax.plot(curve2[:, 0], curve2[:, 1], linewidth=2.0, color=color, alpha=0.9)
            ax.plot(edge[:, 0], edge[:, 1], linewidth=2.0, color=color, alpha=0.9)


def draw_circle(ax, circle, color="deepskyblue", linestyle="-", linewidth=1.8, show_center=False):
    if not all(k in circle for k in ("x", "y", "a", "b")):
        return

    ax.add_patch(Ellipse(
        xy=(circle["x"], circle["y"]),
        width=2 * circle["a"],
        height=2 * circle["b"],
        fill=False,
        edgecolor=color,
        linestyle=linestyle,
        linewidth=linewidth,
        alpha=0.98,
        zorder=4,
    ))

    if show_center:
        ax.plot(circle["x"], circle["y"], "o", color=color, markersize=2.5, zorder=5)


def draw_all_circles(ax, data, frame_idx, show_centers=False):
    for circle in data.get("moving_circles", [])[frame_idx]:
        draw_circle(ax, circle, color="deepskyblue", linestyle="-", linewidth=1.8, show_center=show_centers)

    for circle in data.get("static_circles", [])[frame_idx]:
        draw_circle(ax, circle, color="yellow", linestyle="--", linewidth=2.0, show_center=show_centers)

    for chain in data.get("moving_chains", [])[frame_idx]:
        for member in chain.get("members", []):
            draw_circle(ax, member, color="lime", linestyle="-", linewidth=1.5, show_center=show_centers)

    for chain in data.get("static_chains", [])[frame_idx]:
        for member in chain.get("members", []):
            draw_circle(ax, member, color="orange", linestyle="--", linewidth=1.9, show_center=show_centers)


# -----------------------------
# Animation shell
# -----------------------------

def save_animation(draw_frame, n_frames, output_path):
    ensure_output_dir(output_path)
    fig, ax = plt.subplots(figsize=FIGSIZE)

    def update(frame_idx):
        ax.clear()
        draw_frame(ax, frame_idx)
        return []

    ani = animation.FuncAnimation(fig, update, frames=n_frames, interval=1000 // FPS, blit=False)
    ani.save(output_path, writer=animation.FFMpegWriter(fps=FPS), dpi=DPI)
    plt.close(fig)


# -----------------------------
# Main plot types
# -----------------------------

def animate_circles(data, output_path="output/circles_animation.mp4"):
    n_frames = get_frame_count(data)
    width = data["width"]
    height = data["height"]

    def draw(ax, frame_idx):
        setup_axes(ax, width, height, "Circle Positions vs Time")
        draw_electrodes(ax, data.get("electrodes", []))
        draw_all_circles(ax, data, frame_idx)
        add_frame_label(ax, frame_idx)

    save_animation(draw, n_frames, output_path)


def animate_potential(data, output_path="output/potential_animation.mp4", heatmap=None):
    if heatmap is None:
        heatmap = make_heatmap_settings(mode="full", cmap="coolwarm")

    potential = data["potential"]
    n_frames, height, width = potential.shape
    norm = make_norm(potential, heatmap)

    ensure_output_dir(output_path)
    fig, ax = plt.subplots(figsize=FIGSIZE)
    im = draw_heatmap(ax, potential[0], norm, heatmap)
    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label("Potential")

    def update(frame_idx):
        ax.clear()
        setup_axes(ax, width, height, "Potential + Circles vs Time")
        draw_heatmap(ax, potential[frame_idx], norm, heatmap)
        draw_electrodes(ax, data.get("electrodes", []))
        draw_all_circles(ax, data, frame_idx)
        add_frame_label(ax, frame_idx, extra_text=f"{heatmap['mode']} [{heatmap['low']}, {heatmap['high']}]")
        return []

    ani = animation.FuncAnimation(fig, update, frames=n_frames, interval=1000 // FPS, blit=False)
    ani.save(output_path, writer=animation.FFMpegWriter(fps=FPS), dpi=DPI)
    plt.close(fig)


def animate_fieldmag_with_circles(data, output_path="output/fieldmag_with_circles.mp4", heatmap=None):
    if heatmap is None:
        heatmap = make_heatmap_settings(mode="log", cmap="inferno", low=1, high=99)

    field_mag = data["field_mag"]
    n_frames, height, width = field_mag.shape
    norm = make_norm(field_mag, heatmap)

    ensure_output_dir(output_path)
    fig, ax = plt.subplots(figsize=FIGSIZE)
    im = draw_heatmap(ax, field_mag[0], norm, heatmap)
    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label("|E|")

    def update(frame_idx):
        ax.clear()
        setup_axes(ax, width, height, "Electric Field Magnitude + Circles vs Time")
        draw_heatmap(ax, field_mag[frame_idx], norm, heatmap)
        draw_electrodes(ax, data.get("electrodes", []))
        draw_all_circles(ax, data, frame_idx)
        add_frame_label(ax, frame_idx, extra_text=f"{heatmap['mode']} [{heatmap['low']}, {heatmap['high']}]")
        return []

    ani = animation.FuncAnimation(fig, update, frames=n_frames, interval=1000 // FPS, blit=False)
    ani.save(output_path, writer=animation.FFMpegWriter(fps=FPS), dpi=DPI)
    plt.close(fig)


def animate_field_quiver(data, output_path="output/efield_quiver_animation.mp4", stride=20,
                         normalize=True, background_heatmap=None):
    if background_heatmap is None:
        background_heatmap = make_heatmap_settings(mode="log", cmap="gray", low=5, high=95, alpha=0.25)

    field_x = data["field_x"]
    field_y = data["field_y"]
    n_frames, height, width = field_x.shape
    y, x = np.mgrid[0:height:stride, 0:width:stride]
    bg_norm = make_norm(data["field_mag"], background_heatmap) if "field_mag" in data else None

    def draw(ax, frame_idx):
        setup_axes(ax, width, height, "Electric Field Quiver vs Time")

        if "field_mag" in data:
            draw_heatmap(ax, data["field_mag"][frame_idx], bg_norm, background_heatmap)

        draw_electrodes(ax, data.get("electrodes", []))

        u = field_x[frame_idx, ::stride, ::stride]
        v = field_y[frame_idx, ::stride, ::stride]
        if normalize:
            mag = np.sqrt(u ** 2 + v ** 2)
            u = np.divide(u, mag, out=np.zeros_like(u), where=mag > 1e-12)
            v = np.divide(v, mag, out=np.zeros_like(v), where=mag > 1e-12)

        ax.quiver(
            x, y, u, v,
            angles="xy",
            scale_units="xy",
            scale=0.075 if normalize else None,
            pivot="mid",
            width=0.0028,
            color="deepskyblue",
            alpha=0.95,
            zorder=3,
        )

        draw_all_circles(ax, data, frame_idx)
        add_frame_label(ax, frame_idx, extra_text=f"bg {background_heatmap['mode']} [{background_heatmap['low']}, {background_heatmap['high']}]")

    save_animation(draw, n_frames, output_path)


def animate_field_lines(data, output_path="output/field_lines_animation.mp4", density=1.0,
                        background_heatmap=None):
    if background_heatmap is None:
        background_heatmap = make_heatmap_settings(mode="log", cmap="magma", low=1, high=99, alpha=0.70)

    field_x = data["field_x"]
    field_y = data["field_y"]
    n_frames, height, width = field_x.shape
    x = np.arange(0, width)
    y = np.arange(0, height)
    bg_norm = make_norm(data["field_mag"], background_heatmap) if "field_mag" in data else None

    def draw(ax, frame_idx):
        setup_axes(ax, width, height, "Electric Field Lines vs Time")

        if "field_mag" in data and background_heatmap is not None:
            draw_heatmap(ax, data["field_mag"][frame_idx], bg_norm, background_heatmap)

        draw_electrodes(ax, data.get("electrodes", []))
        ax.streamplot(
            x, y,
            field_x[frame_idx], field_y[frame_idx],
            density=density,
            linewidth=0.8,
            arrowsize=0.8,
            color="white",
            zorder=3,
        )
        draw_all_circles(ax, data, frame_idx)
        label = None if background_heatmap is None else f"bg {background_heatmap['mode']} [{background_heatmap['low']}, {background_heatmap['high']}]"
        add_frame_label(ax, frame_idx, extra_text=label)

    save_animation(draw, n_frames, output_path)


def plot_fieldmag_frame(data, frame_idx=-1, output_path="output/fieldmag_frame.png", heatmap=None):
    if heatmap is None:
        heatmap = make_heatmap_settings(mode="log", cmap="inferno", low=1, high=99)

    ensure_output_dir(output_path)
    field_mag = data["field_mag"]
    n_frames = field_mag.shape[0]
    if frame_idx < 0:
        frame_idx = n_frames + frame_idx

    norm = make_norm(field_mag, heatmap)

    fig, ax = plt.subplots(figsize=FIGSIZE)
    setup_axes(ax, data["width"], data["height"], "Electric Field Magnitude + Circles")
    im = draw_heatmap(ax, field_mag[frame_idx], norm, heatmap)
    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label("|E|")
    draw_electrodes(ax, data.get("electrodes", []))
    draw_all_circles(ax, data, frame_idx)
    add_frame_label(ax, frame_idx, extra_text=f"{heatmap['mode']} [{heatmap['low']}, {heatmap['high']}]")
    fig.tight_layout()
    fig.savefig(output_path, dpi=DPI)
    plt.close(fig)


def plot_resistance_vs_time(data, output_path="output/resistance_vs_time.png"):
    ensure_output_dir(output_path)
    r = np.asarray(data.get("resistance_measurements", []), dtype=float)
    if r.size == 0:
        print("No resistance_measurements found.")
        return

    frames = np.arange(len(r))
    fig, ax = plt.subplots(figsize=(9, 4))
    ax.plot(frames, r)
    ax.set_title("Resistance vs Time")
    ax.set_xlabel("Frame")
    ax.set_ylabel("Resistance")
    fig.tight_layout()
    fig.savefig(output_path, dpi=DPI)
    plt.close(fig)


def split_relax_runs(relax_iterations, relax_error):
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


def plot_relax_error_decay(data, output_path="output/relax_error_decay.png", max_runs=None):
    ensure_output_dir(output_path)
    relax_iterations = data.get("relax_iterations", [])
    relax_error = data.get("relax_error", [])

    if not relax_iterations or not relax_error:
        print("No relax history found.")
        return

    runs = split_relax_runs(relax_iterations, relax_error)
    if max_runs is not None:
        runs = runs[:max_runs]

    fig, ax = plt.subplots(figsize=(9, 5))
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
    fig.savefig(output_path, dpi=DPI)
    plt.close(fig)


def main():
    data = load_sim_data(DATA_PATH)

    # Heatmap presets you can tune directly.
    potential_map = make_heatmap_settings(mode="full", cmap="coolwarm")
    field_low = make_heatmap_settings(mode="log", cmap="inferno", low=10, high=90, alpha=1.0)
    field_high = make_heatmap_settings(mode="percentile", cmap="inferno", low=10, high=99.5, alpha=1.0)
    quiver_bg = make_heatmap_settings(mode="log", cmap="gray", low=5, high=95, alpha=0.25)
    lines_bg = make_heatmap_settings(mode="percentile", cmap="magma", low=0.1, high=99, alpha=0.80)

    plot_fieldmag_frame(data, frame_idx=-1, output_path=OUTPUT_DIR / "fieldmag_last_frame_low.png", heatmap=field_low)
    plot_fieldmag_frame(data, frame_idx=-1, output_path=OUTPUT_DIR / "fieldmag_last_frame_high.png", heatmap=field_high)
    plot_relax_error_decay(data, output_path=OUTPUT_DIR / "relax_error_decay.png")
    plot_resistance_vs_time(data, output_path=OUTPUT_DIR / "resistance_vs_time.png")

    animate_circles(data, output_path=OUTPUT_DIR / "circles_animation.mp4")
    animate_potential(data, output_path=OUTPUT_DIR / "potential_animation.mp4", heatmap=potential_map)
    animate_fieldmag_with_circles(data, output_path=OUTPUT_DIR / "fieldmag_with_circles_low.mp4", heatmap=field_low)
    animate_fieldmag_with_circles(data, output_path=OUTPUT_DIR / "fieldmag_with_circles_high.mp4", heatmap=field_high)
    # animate_field_quiver(data, output_path=OUTPUT_DIR / "efield_quiver_animation.mp4", stride=20, normalize=True, background_heatmap=quiver_bg)
    animate_field_lines(data, output_path=OUTPUT_DIR / "field_lines_animation.mp4", density=1.0, background_heatmap=lines_bg)


if __name__ == "__main__":
    main()
