import React from 'react';

export type BadgeTone = 'emerald' | 'amber' | 'sky' | 'rose' | 'neutral' | 'violet';

interface StatusBadgeProps {
  label: string;
  tone?: BadgeTone;
  dot?: boolean;
  pulse?: boolean;
  className?: string;
  id?: string;
}

export const StatusBadge: React.FC<StatusBadgeProps> = ({
  label,
  tone = 'neutral',
  dot = true,
  pulse = false,
  className = '',
  id,
}) => {
  const toneClasses: Record<BadgeTone, { bg: string; text: string; dot: string; border: string }> = {
    emerald: {
      bg: 'bg-emerald-500/10 dark:bg-emerald-500/15',
      text: 'text-emerald-700 dark:text-emerald-300',
      dot: 'bg-emerald-500 dark:bg-emerald-400',
      border: 'border-emerald-500/20 dark:border-emerald-500/30',
    },
    amber: {
      bg: 'bg-amber-500/10 dark:bg-amber-500/15',
      text: 'text-amber-700 dark:text-amber-300',
      dot: 'bg-amber-500 dark:bg-amber-400',
      border: 'border-amber-500/20 dark:border-amber-500/30',
    },
    sky: {
      bg: 'bg-sky-500/10 dark:bg-sky-500/15',
      text: 'text-sky-700 dark:text-sky-300',
      dot: 'bg-sky-500 dark:bg-sky-400',
      border: 'border-sky-500/20 dark:border-sky-500/30',
    },
    rose: {
      bg: 'bg-rose-500/10 dark:bg-rose-500/15',
      text: 'text-rose-700 dark:text-rose-300',
      dot: 'bg-rose-500 dark:bg-rose-400',
      border: 'border-rose-500/20 dark:border-rose-500/30',
    },
    violet: {
      bg: 'bg-violet-500/10 dark:bg-violet-500/15',
      text: 'text-violet-700 dark:text-violet-300',
      dot: 'bg-violet-500 dark:bg-violet-400',
      border: 'border-violet-500/20 dark:border-violet-500/30',
    },
    neutral: {
      bg: 'bg-neutral-500/10 dark:bg-neutral-400/10',
      text: 'text-neutral-700 dark:text-neutral-300',
      dot: 'bg-neutral-400 dark:bg-neutral-400',
      border: 'border-neutral-400/20 dark:border-neutral-500/20',
    },
  };

  const selectedTone = toneClasses[tone];

  return (
    <span
      id={id}
      className={`inline-flex items-center gap-1.5 px-2.5 py-0.5 rounded-full text-xs font-medium border ${selectedTone.bg} ${selectedTone.text} ${selectedTone.border} transition-colors ${className}`}
    >
      {dot && (
        <span
          className={`w-1.5 h-1.5 rounded-full shrink-0 ${selectedTone.dot} ${
            pulse ? 'animate-pulse' : ''
          }`}
          aria-hidden="true"
        />
      )}
      <span>{label}</span>
    </span>
  );
};
