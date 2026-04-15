import json
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.colors import LogNorm
from matplotlib.patches import Ellipse, Polygon, Rectangle


DATA_PATH = Path("output/data.json")
OUTPUT_DIR = Path("output")
FPS = 5
FIGSIZE = (10, 7)
DPI = 150


# -----------------------------
# Data loading and validation
# -----------------------------

def load_sim_data(path: Path):
    with path.open("r", encoding="utf-8") as f:
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


def get_frame_count(data):
    candidates = []
    for key in ["potential", "field_mag", "field_x", "field_y"]:
        arr = data.get(key)
        if arr is not None:
            candidates.append(arr.shape[0])

    for key in ["moving_circles", "static_circles", "moving_chains", "static_chains"]:
        seq = data.get(key)
        if seq:
            candidates.append(len(seq))

    if not candidates:
        raise ValueError("No frame-based data found in JSON.")

    return min(candidates)


# -----------------------------
# Styling and axes helpers
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
# Electrode drawing
# -----------------------------

def electrode_style(value):
    if value > 0:
        return {"color": "red", "linewidth": 2.0, "alpha": 0.9}
    if value < 0:
        return {"color": "cyan", "linewidth": 2.0, "alpha": 0.9}
    return {"color": "white", "linewidth": 2.0, "alpha": 0.9}


def draw_electrodes(ax, electrodes):
    for elec in electrodes:
        shape = elec["shape"]
        style = electrode_style(elec.get("value", 0.0))

        if shape == "triangle":
            pts = np.array([elec["v1"], elec["v2"], elec["v3"]], dtype=float)
            poly = Polygon(pts, closed=True, fill=False, **style)
            ax.add_patch(poly)

        elif shape == "rectangle":
            pos = elec["position"]
            size = elec["size"]
            rect = Rectangle(
                xy=(pos[0], pos[1]),
                width=size[0],
                height=size[1],
                fill=False,
                **style,
            )
            ax.add_patch(rect)

        elif shape == "needle":
            if "edge_points" in elec and elec["edge_points"]:
                edge = np.asarray(elec["edge_points"], dtype=float)
                if edge.ndim == 2 and edge.shape[1] == 2:
                    ax.plot(edge[:, 0], edge[:, 1], linestyle="", marker=".", markersize=1.0,
                            color=style["color"], alpha=0.7)
                    continue

            v1 = np.array(elec["v1"], dtype=float)
            v2 = np.array(elec["v2"], dtype=float)
            v3 = np.array(elec["v3"], dtype=float)
            cp = np.array(elec["cp"], dtype=float)

            t = np.linspace(0.0, 1.0, 300)
            curve1 = ((1 - t)[:, None] ** 2) * v1 + 2 * ((1 - t)[:, None] * t[:, None]) * cp + (t[:, None] ** 2) * v3
            curve2 = ((1 - t)[:, None] ** 2) * v2 + 2 * ((1 - t)[:, None] * t[:, None]) * cp + (t[:, None] ** 2) * v3
            edge = np.vstack([v1, v2])

            ax.plot(curve1[:, 0], curve1[:, 1], **style)
            ax.plot(curve2[:, 0], curve2[:, 1], **style)
            ax.plot(edge[:, 0], edge[:, 1], **style)


# -----------------------------
# Circle and chain overlays
# -----------------------------

def _iter_all_circle_dicts(data, frame_idx):
    for c in data.get("moving_circles", [])[frame_idx]:
        yield c, "moving"

    for c in data.get("static_circles", [])[frame_idx]:
        yield c, "static"

    for chain in data.get("moving_chains", [])[frame_idx]:
        for member in chain.get("members", []):
            yield member, "moving_chain"

    for chain in data.get("static_chains", [])[frame_idx]:
        for member in chain.get("members", []):
            yield member, "static_chain"


def circle_style(kind):
    if kind == "moving":
        return {"linestyle": "-", "linewidth": 1.6, "edgecolor": "white", "alpha": 0.95}
    if kind == "static":
        return {"linestyle": "--", "linewidth": 2.0, "edgecolor": "yellow", "alpha": 0.95}
    if kind == "moving_chain":
        return {"linestyle": "-", "linewidth": 1.2, "edgecolor": "lime", "alpha": 0.90}
    if kind == "static_chain":
        return {"linestyle": "--", "linewidth": 1.8, "edgecolor": "orange", "alpha": 0.95}
    return {"linestyle": "-", "linewidth": 1.5, "edgecolor": "white", "alpha": 0.95}


def draw_circle_overlay(ax, data, frame_idx, show_centers=False, show_ids=False):
    for circle_dict, kind in _iter_all_circle_dicts(data, frame_idx):
        style = circle_style(kind)
        patch = Ellipse(
            xy=(circle_dict["x"], circle_dict["y"]),
            width=2 * circle_dict["a"],
            height=2 * circle_dict["b"],
            fill=False,
            linestyle=style["linestyle"],
            linewidth=style["linewidth"],
            edgecolor=style["edgecolor"],
            alpha=style["alpha"],
        )
        ax.add_patch(patch)

        if show_centers:
            ax.plot(circle_dict["x"], circle_dict["y"], marker="o", markersize=2.0,
                    color=style["edgecolor"], alpha=style["alpha"])

        if show_ids and "id" in circle_dict:
            ax.text(circle_dict["x"], circle_dict["y"], str(circle_dict["id"]),
                    fontsize=6, color=style["edgecolor"], ha="center", va="center")


# -----------------------------
# Scalar field helpers
# -----------------------------

def compute_percentile_limits(stack, low=1.0, high=99.0):
    arr = np.asarray(stack, dtype=float)
    finite = arr[np.isfinite(arr)]
    if finite.size == 0:
        return 0.0, 1.0
    vmin = np.percentile(finite, low)
    vmax = np.percentile(finite, high)
    if vmax <= vmin:
        vmax = vmin + 1.0
    return float(vmin), float(vmax)


def get_scalar_norm(stack, mode="linear", low_pct=1.0, high_pct=99.0):
    vmin, vmax = compute_percentile_limits(stack, low=low_pct, high=high_pct)

    if mode == "log":
        positive = np.asarray(stack, dtype=float)
        positive = positive[np.isfinite(positive) & (positive > 0)]
        if positive.size == 0:
            return None, vmin, vmax
        vmin = max(float(np.percentile(positive, low_pct)), 1e-12)
        vmax = max(float(np.percentile(positive, high_pct)), vmin * 10.0)
        return LogNorm(vmin=vmin, vmax=vmax), vmin, vmax

    return None, vmin, vmax


def draw_scalar_field(ax, frame2d, title, cmap="viridis", norm_mode="linear",
                      reference_stack=None, colorbar_label=None):
    stack = frame2d[None, ...] if reference_stack is None else reference_stack
    norm, vmin, vmax = get_scalar_norm(stack, mode=norm_mode)

    im_kwargs = {
        "origin": "lower",
        "aspect": "auto",
        "cmap": cmap,
    }

    if norm is not None:
        im_kwargs["norm"] = norm
    else:
        im_kwargs["vmin"] = vmin
        im_kwargs["vmax"] = vmax

    im = ax.imshow(frame2d, **im_kwargs)
    ax.set_title(title)

    if colorbar_label is not None:
        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label(colorbar_label)

    return im


# -----------------------------
# Reusable frame rendering helpers
# -----------------------------

def render_circle_frame(ax, data, frame_idx, title="Circle Positions vs Time",
                        show_centers=False, show_ids=False):
    width = data["width"]
    height = data["height"]
    setup_axes(ax, width, height, title)
    draw_electrodes(ax, data.get("electrodes", []))
    draw_circle_overlay(ax, data, frame_idx, show_centers=show_centers, show_ids=show_ids)
    add_frame_label(ax, frame_idx, color="white")


def render_scalar_with_circles(ax, data, scalar_key, frame_idx, title,
                               cmap="viridis", norm_mode="linear",
                               colorbar_label=None, show_centers=False,
                               add_colorbar=False):
    width = data["width"]
    height = data["height"]
    setup_axes(ax, width, height, title)

    stack = data[scalar_key]
    im = draw_scalar_field(
        ax,
        stack[frame_idx],
        title=title,
        cmap=cmap,
        norm_mode=norm_mode,
        reference_stack=stack,
        colorbar_label=colorbar_label if add_colorbar else None,
    )

    draw_electrodes(ax, data.get("electrodes", []))
    draw_circle_overlay(ax, data, frame_idx, show_centers=show_centers)
    add_frame_label(ax, frame_idx, color="white")
    return im


# -----------------------------
# Generic animation writer
# -----------------------------

def animate_redraw(draw_frame_func, n_frames, output_path, figsize=FIGSIZE, fps=FPS, dpi=DPI):
    ensure_output_dir(output_path)

    fig, ax = plt.subplots(figsize=figsize)

    def update(frame_idx):
        ax.clear()
        artists = draw_frame_func(ax, frame_idx)
        if artists is None:
            artists = []
        return artists

    ani = animation.FuncAnimation(
        fig,
        update,
        frames=n_frames,
        interval=1000 // fps,
        blit=False,
    )
    ani.save(output_path, writer=animation.FFMpegWriter(fps=fps), dpi=dpi)
    plt.close(fig)


# -----------------------------
# Plotting / animation entry points
# -----------------------------

def animate_circles(data, output_path="output/circles_animation.mp4",
                    show_centers=False, show_ids=False):
    n_frames = get_frame_count(data)

    def draw(ax, frame_idx):
        render_circle_frame(
            ax,
            data,
            frame_idx,
            title="Circle Positions vs Time",
            show_centers=show_centers,
            show_ids=show_ids,
        )
        return []

    animate_redraw(draw, n_frames=n_frames, output_path=output_path)


def animate_potential(data, output_path="output/potential_animation.mp4",
                      cmap="coolwarm"):
    potential = data["potential"]
    n_frames = potential.shape[0]

    def draw(ax, frame_idx):
        render_scalar_with_circles(
            ax,
            data,
            scalar_key="potential",
            frame_idx=frame_idx,
            title="Potential + Circles vs Time",
            cmap=cmap,
            norm_mode="linear",
            colorbar_label="Potential",
            add_colorbar=True,
        )
        return []

    animate_redraw(draw, n_frames=n_frames, output_path=output_path)


def animate_fieldmag_with_circles(data, output_path="output/fieldmag_with_circles.mp4",
                                  cmap="inferno", norm_mode="log"):
    field_mag = data["field_mag"]
    n_frames = field_mag.shape[0]

    def draw(ax, frame_idx):
        render_scalar_with_circles(
            ax,
            data,
            scalar_key="field_mag",
            frame_idx=frame_idx,
            title="Electric Field Magnitude + Circles vs Time",
            cmap=cmap,
            norm_mode=norm_mode,
            colorbar_label="|E|",
            add_colorbar=True,
        )
        return []

    animate_redraw(draw, n_frames=n_frames, output_path=output_path)


def plot_fieldmag_frame(data, frame_idx=-1,
                        output_path="output/fieldmag_frame.png",
                        cmap="inferno", norm_mode="log"):
    ensure_output_dir(output_path)
    n_frames = data["field_mag"].shape[0]
    if frame_idx < 0:
        frame_idx = n_frames + frame_idx

    fig, ax = plt.subplots(figsize=FIGSIZE)
    render_scalar_with_circles(
        ax,
        data,
        scalar_key="field_mag",
        frame_idx=frame_idx,
        title="Electric Field Magnitude + Circles",
        cmap=cmap,
        norm_mode=norm_mode,
        colorbar_label="|E|",
        add_colorbar=True,
    )
    fig.tight_layout()
    fig.savefig(output_path, dpi=DPI)
    plt.close(fig)


def normalize_vectors(u, v, eps=1e-12):
    mag = np.sqrt(u ** 2 + v ** 2)
    u_norm = np.zeros_like(u, dtype=float)
    v_norm = np.zeros_like(v, dtype=float)
    np.divide(u, mag, out=u_norm, where=mag > eps)
    np.divide(v, mag, out=v_norm, where=mag > eps)
    return u_norm, v_norm


def animate_field_quiver(data, output_path="output/efield_quiver_animation.mp4",
                         stride=20, normalize=True, overlay_field_mag=True,
                         fieldmag_cmap="gray", fieldmag_alpha=0.45,
                         fieldmag_norm_mode="log"):
    field_x = data["field_x"]
    field_y = data["field_y"]
    n_frames, height, width = field_x.shape
    y, x = np.mgrid[0:height:stride, 0:width:stride]

    def draw(ax, frame_idx):
        setup_axes(ax, width, height, "Electric Field Quiver vs Time")

        if overlay_field_mag and "field_mag" in data:
            stack = data["field_mag"]
            norm, vmin, vmax = get_scalar_norm(stack, mode=fieldmag_norm_mode)
            im_kwargs = {
                "origin": "lower",
                "aspect": "auto",
                "cmap": fieldmag_cmap,
                "alpha": fieldmag_alpha,
            }
            if norm is not None:
                im_kwargs["norm"] = norm
            else:
                im_kwargs["vmin"] = vmin
                im_kwargs["vmax"] = vmax
            ax.imshow(stack[frame_idx], **im_kwargs)

        draw_electrodes(ax, data.get("electrodes", []))

        u = field_x[frame_idx, ::stride, ::stride]
        v = field_y[frame_idx, ::stride, ::stride]
        if normalize:
            u, v = normalize_vectors(u, v)

        ax.quiver(
            x, y, u, v,
            angles="xy",
            scale_units="xy",
            scale=0.07 if normalize else None,
            pivot="mid",
            width=0.0025,
            color="white" if overlay_field_mag else None,
        )

        draw_circle_overlay(ax, data, frame_idx)
        add_frame_label(ax, frame_idx, color="white")
        return []

    animate_redraw(draw, n_frames=n_frames, output_path=output_path)


def plot_field_lines_frame(data, frame_idx=0,
                           output_path="output/field_lines_frame.png",
                           density=1.2, cmap="viridis", overlay_field_mag=True):
    ensure_output_dir(output_path)
    field_x = data["field_x"][frame_idx]
    field_y = data["field_y"][frame_idx]
    height, width = field_x.shape

    x = np.arange(0, width)
    y = np.arange(0, height)

    fig, ax = plt.subplots(figsize=FIGSIZE)
    setup_axes(ax, width, height, f"Electric Field Lines (Frame {frame_idx})")

    if overlay_field_mag and "field_mag" in data:
        draw_scalar_field(
            ax,
            data["field_mag"][frame_idx],
            title=f"Electric Field Lines (Frame {frame_idx})",
            cmap="magma",
            norm_mode="log",
            reference_stack=data["field_mag"],
            colorbar_label="|E|",
        )

    draw_electrodes(ax, data.get("electrodes", []))
    ax.streamplot(x, y, field_x, field_y, density=density, linewidth=0.8, arrowsize=0.8, color="white")
    draw_circle_overlay(ax, data, frame_idx)
    add_frame_label(ax, frame_idx, color="white")

    fig.tight_layout()
    fig.savefig(output_path, dpi=DPI)
    plt.close(fig)


# NOTE:
# Animated streamplot is intentionally implemented with full axes redraw every frame.
# This avoids the fragile artist-removal path that often breaks saving.

def animate_field_lines(data, output_path="output/field_lines_animation.mp4",
                        density=1.0, overlay_field_mag=True):
    field_x = data["field_x"]
    field_y = data["field_y"]
    n_frames, height, width = field_x.shape
    x = np.arange(0, width)
    y = np.arange(0, height)

    def draw(ax, frame_idx):
        setup_axes(ax, width, height, "Electric Field Lines vs Time")

        if overlay_field_mag and "field_mag" in data:
            stack = data["field_mag"]
            norm, vmin, vmax = get_scalar_norm(stack, mode="log")
            im_kwargs = {
                "origin": "lower",
                "aspect": "auto",
                "cmap": "magma",
                "alpha": 0.9,
            }
            if norm is not None:
                im_kwargs["norm"] = norm
            else:
                im_kwargs["vmin"] = vmin
                im_kwargs["vmax"] = vmax
            ax.imshow(stack[frame_idx], **im_kwargs)
            line_color = "white"
        else:
            line_color = None

        draw_electrodes(ax, data.get("electrodes", []))
        ax.streamplot(
            x,
            y,
            field_x[frame_idx],
            field_y[frame_idx],
            density=density,
            linewidth=0.8,
            arrowsize=0.8,
            color=line_color,
        )
        draw_circle_overlay(ax, data, frame_idx)
        add_frame_label(ax, frame_idx, color="white")
        return []

    animate_redraw(draw, n_frames=n_frames, output_path=output_path)


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

    # Static diagnostics
    plot_fieldmag_frame(data, frame_idx=-1, output_path=OUTPUT_DIR / "fieldmag_last_frame.png")
    plot_field_lines_frame(data, frame_idx=0, output_path=OUTPUT_DIR / "field_lines_frame0.png")
    plot_relax_error_decay(data, output_path=OUTPUT_DIR / "relax_error_decay.png")
    plot_resistance_vs_time(data, output_path=OUTPUT_DIR / "resistance_vs_time.png")

    # Animations
    animate_circles(data, output_path=OUTPUT_DIR / "circles_animation.mp4")
    animate_potential(data, output_path=OUTPUT_DIR / "potential_animation.mp4")
    animate_fieldmag_with_circles(data, output_path=OUTPUT_DIR / "fieldmag_with_circles.mp4")
    animate_field_quiver(data, output_path=OUTPUT_DIR / "efield_quiver_animation.mp4", stride=20)
    animate_field_lines(data, output_path=OUTPUT_DIR / "field_lines_animation.mp4", density=1.0)


if __name__ == "__main__":
    main()
