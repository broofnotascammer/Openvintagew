import React from 'react';

interface GlassPanelProps {
  children: React.ReactNode;
  className?: string;
  id?: string;
  isInteractive?: boolean;
  onClick?: () => void;
  title?: string;
  action?: React.ReactNode;
}

export const GlassPanel: React.FC<GlassPanelProps> = ({
  children,
  className = '',
  id,
  isInteractive = false,
  onClick,
  title,
  action,
}) => {
  const interactiveClass = isInteractive
    ? 'glass-card-interactive-light dark:glass-card-interactive-dark cursor-pointer'
    : '';

  return (
    <div
      id={id}
      onClick={onClick}
      className={`glass-card-light dark:glass-card-dark rounded-xl p-5 ${interactiveClass} ${className}`}
    >
      {(title || action) && (
        <div className="flex items-center justify-between pb-3 mb-4 border-b border-black/5 dark:border-white/5">
          {title && (
            <h3 className="text-sm font-semibold tracking-tight text-neutral-900 dark:text-neutral-100 flex items-center gap-2">
              {title}
            </h3>
          )}
          {action && <div>{action}</div>}
        </div>
      )}
      {children}
    </div>
  );
};

interface MetricCardProps {
  id?: string;
  label: string;
  value: string | number;
  subValue?: string;
  icon?: React.ReactNode;
  statusBadge?: React.ReactNode;
  onClick?: () => void;
  className?: string;
}

export const MetricCard: React.FC<MetricCardProps> = ({
  id,
  label,
  value,
  subValue,
  icon,
  statusBadge,
  onClick,
  className = '',
}) => {
  return (
    <div
      id={id}
      onClick={onClick}
      className={`glass-card-light dark:glass-card-dark rounded-xl p-4 flex flex-col justify-between ${
        onClick ? 'glass-card-interactive-light dark:glass-card-interactive-dark cursor-pointer' : ''
      } ${className}`}
    >
      <div className="flex items-start justify-between gap-2 mb-2">
        <span className="text-xs font-medium text-neutral-500 dark:text-neutral-400">
          {label}
        </span>
        {icon && (
          <div className="text-neutral-500 dark:text-neutral-400 shrink-0">
            {icon}
          </div>
        )}
      </div>

      <div>
        <div className="text-lg font-semibold text-neutral-900 dark:text-neutral-100 truncate">
          {value}
        </div>
        {(subValue || statusBadge) && (
          <div className="mt-1.5 flex items-center justify-between gap-2 flex-wrap">
            {subValue && (
              <span className="text-xs text-neutral-500 dark:text-neutral-400 truncate">
                {subValue}
              </span>
            )}
            {statusBadge && <div className="shrink-0">{statusBadge}</div>}
          </div>
        )}
      </div>
    </div>
  );
};
